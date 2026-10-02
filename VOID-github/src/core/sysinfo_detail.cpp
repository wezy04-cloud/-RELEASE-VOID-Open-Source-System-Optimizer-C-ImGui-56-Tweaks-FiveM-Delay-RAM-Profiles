#include "sysinfo_detail.hpp"
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <dxgi.h>
#include <cstdio>
#include <ctime>

#pragma comment(lib, "dxgi.lib")

namespace sysdetail
{
    namespace
    {
        Info g_info;
        bool g_gathered = false;

        std::string RegStr(HKEY root, const wchar_t* path, const wchar_t* name)
        {
            HKEY key;
            if (RegOpenKeyExW(root, path, 0, KEY_READ, &key) != ERROR_SUCCESS)
                return {};
            wchar_t buf[512] = {};
            DWORD sz = sizeof(buf), type = 0;
            RegQueryValueExW(key, name, nullptr, &type, (LPBYTE)buf, &sz);
            RegCloseKey(key);
            if (type != REG_SZ && type != REG_EXPAND_SZ) return {};
            char out[512];
            WideCharToMultiByte(CP_UTF8, 0, buf, -1, out, sizeof(out), nullptr, nullptr);
            return out;
        }

        DWORD RegDword(HKEY root, const wchar_t* path, const wchar_t* name)
        {
            HKEY key;
            if (RegOpenKeyExW(root, path, 0, KEY_READ, &key) != ERROR_SUCCESS)
                return 0;
            DWORD val = 0, sz = sizeof(val), type = 0;
            RegQueryValueExW(key, name, nullptr, &type, (LPBYTE)&val, &sz);
            RegCloseKey(key);
            return val;
        }
    }

    void Gather()
    {
        if (g_gathered) return;
        g_gathered = true;

        // CPU cores & threads
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        g_info.cpuThreads = (int)si.dwNumberOfProcessors;

        DWORD len = 0;
        GetLogicalProcessorInformation(nullptr, &len);
        if (len > 0)
        {
            auto* buf = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION*)malloc(len);
            if (buf && GetLogicalProcessorInformation(buf, &len))
            {
                int cores = 0;
                DWORD count = len / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
                for (DWORD i = 0; i < count; ++i)
                    if (buf[i].Relationship == RelationProcessorCore)
                        cores++;
                g_info.cpuCores = cores;
            }
            free(buf);
        }
        if (g_info.cpuCores == 0) g_info.cpuCores = g_info.cpuThreads;

        // CPU clock from registry
        DWORD mhz = RegDword(HKEY_LOCAL_MACHINE,
            L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", L"~MHz");
        if (mhz > 0)
        {
            char buf[32];
            snprintf(buf, sizeof(buf), "%.2f GHz", mhz / 1000.0);
            g_info.cpuClock = buf;
        }
        else
            g_info.cpuClock = "N/A";

        // GPU VRAM via DXGI
        {
            IDXGIFactory* factory = nullptr;
            if (SUCCEEDED(CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory)))
            {
                IDXGIAdapter* adapter = nullptr;
                if (SUCCEEDED(factory->EnumAdapters(0, &adapter)))
                {
                    DXGI_ADAPTER_DESC desc;
                    adapter->GetDesc(&desc);
                    SIZE_T mb = desc.DedicatedVideoMemory / (1024 * 1024);
                    char buf[32];
                    if (mb >= 1024)
                        snprintf(buf, sizeof(buf), "%.0f GB", mb / 1024.0);
                    else
                        snprintf(buf, sizeof(buf), "%zu MB", mb);
                    g_info.gpuVram = buf;
                    adapter->Release();
                }
                factory->Release();
            }
            if (g_info.gpuVram.empty()) g_info.gpuVram = "N/A";
        }

        // GPU driver version from registry
        g_info.gpuDriver = RegStr(HKEY_LOCAL_MACHINE,
            L"SOFTWARE\\Microsoft\\DirectX", L"Version");
        if (g_info.gpuDriver.empty())
            g_info.gpuDriver = "N/A";

        // RAM info
        {
            MEMORYSTATUSEX ms = { sizeof(ms) };
            GlobalMemoryStatusEx(&ms);
            char buf[32];
            snprintf(buf, sizeof(buf), "%.0f GB", ms.ullTotalPhys / (1024.0 * 1024.0 * 1024.0));
            g_info.ramTotal = buf;
        }

        // RAM speed from WMI is complex, use registry hint
        g_info.ramSpeed = RegStr(HKEY_LOCAL_MACHINE,
            L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"BIOSReleaseDate");
        if (g_info.ramSpeed.empty()) g_info.ramSpeed = "N/A";
        // Overwrite: we'll try to compute slots from processor count etc. Keep simple.
        g_info.ramSpeed = "N/A";
        g_info.ramSlots = "N/A";

        // Motherboard
        g_info.motherboard = RegStr(HKEY_LOCAL_MACHINE,
            L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"BaseBoardProduct");
        if (g_info.motherboard.empty())
            g_info.motherboard = RegStr(HKEY_LOCAL_MACHINE,
                L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"SystemProductName");
        if (g_info.motherboard.empty())
            g_info.motherboard = "N/A";

        // BIOS version
        g_info.biosVersion = RegStr(HKEY_LOCAL_MACHINE,
            L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"BIOSVersion");
        if (g_info.biosVersion.empty())
            g_info.biosVersion = RegStr(HKEY_LOCAL_MACHINE,
                L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"SystemBiosVersion");
        if (g_info.biosVersion.empty())
            g_info.biosVersion = "N/A";

        // BIOS mode (UEFI / Legacy) - use GetFirmwareEnvironmentVariable probe
        {
            // If GetFirmwareEnvironmentVariable succeeds or returns ERROR_NOACCESS, we're in UEFI
            // If ERROR_INVALID_FUNCTION, we're in Legacy BIOS
            SetLastError(0);
            GetFirmwareEnvironmentVariableA("", "{00000000-0000-0000-0000-000000000000}", nullptr, 0);
            DWORD err = GetLastError();
            if (err == ERROR_INVALID_FUNCTION)
                g_info.biosMode = "Legacy (BIOS)";
            else
                g_info.biosMode = "UEFI";
        }

        // Secure Boot via GetFirmwareEnvironmentVariable
        {
            BYTE val = 0;
            DWORD result = GetFirmwareEnvironmentVariableA(
                "SecureBoot", "{8be4df61-93ca-11d2-aa0d-00e098032b8c}",
                &val, sizeof(val));
            if (result > 0)
                g_info.secureBoot = val ? "Enabled" : "Disabled";
            else
            {
                DWORD err = GetLastError();
                if (err == ERROR_INVALID_FUNCTION)
                    g_info.secureBoot = "Not supported (Legacy BIOS)";
                else
                    g_info.secureBoot = "N/A (admin required)";
            }
        }

        // Virtualization from registry
        {
            DWORD virt = RegDword(HKEY_LOCAL_MACHINE,
                L"SYSTEM\\CurrentControlSet\\Control\\DeviceGuard", L"EnableVirtualizationBasedSecurity");
            std::string hvPresent = RegStr(HKEY_LOCAL_MACHINE,
                L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Virtualization", L"MinVmVersionForCpuBasedMitigations");

            bool isHyperV = false;
            HKEY key;
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Virtualization", 0, KEY_READ, &key) == ERROR_SUCCESS)
            {
                isHyperV = true;
                RegCloseKey(key);
            }

            if (virt == 1 || isHyperV)
                g_info.virtualization = "Enabled";
            else
                g_info.virtualization = "Available";
        }

        // Install date
        {
            DWORD ts = RegDword(HKEY_LOCAL_MACHINE,
                L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"InstallDate");
            if (ts > 0)
            {
                time_t t = (time_t)ts;
                struct tm tm = {};
                errno_t e = ::localtime_s(&tm, &t);
                char buf[32];
                if (e == 0)
                    snprintf(buf, sizeof(buf), "%04d-%02d-%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
                else
                    snprintf(buf, sizeof(buf), "N/A");
                g_info.installDate = buf;
            }
            else
                g_info.installDate = "N/A";
        }

        // DirectX version (approximate from feature level)
        {
            DWORD dxVer = RegDword(HKEY_LOCAL_MACHINE,
                L"SOFTWARE\\Microsoft\\DirectX", L"Version");
            std::string dxStr = RegStr(HKEY_LOCAL_MACHINE,
                L"SOFTWARE\\Microsoft\\DirectX", L"InstalledVersion");
            // Fallback: Windows 10/11 ships with DX12
            g_info.directX = "DirectX 12";
        }

        // Display resolution
        {
            int w = GetSystemMetrics(SM_CXSCREEN);
            int h = GetSystemMetrics(SM_CYSCREEN);
            char buf[32];
            snprintf(buf, sizeof(buf), "%d x %d", w, h);
            g_info.displayRes = buf;
        }

        // System locale
        {
            wchar_t buf[LOCALE_NAME_MAX_LENGTH] = {};
            GetUserDefaultLocaleName(buf, LOCALE_NAME_MAX_LENGTH);
            char out[64];
            WideCharToMultiByte(CP_UTF8, 0, buf, -1, out, sizeof(out), nullptr, nullptr);
            g_info.systemLocale = out;
        }
    }

    const Info& Get()
    {
        if (!g_gathered) Gather();
        return g_info;
    }
}
