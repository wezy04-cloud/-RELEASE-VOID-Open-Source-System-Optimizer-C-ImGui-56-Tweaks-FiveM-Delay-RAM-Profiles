#include "sysinfo.hpp"
#include <windows.h>
#include <dxgi.h>
#include <cstdio>
#include <cstdint>
#include <string>

#pragma comment(lib, "dxgi.lib")

namespace sys
{
    namespace
    {
        Snapshot  g_snap;
        ULONGLONG g_prevIdle = 0, g_prevTotal = 0;

        ULONGLONG ToU64(const FILETIME& ft) { return ((ULONGLONG)ft.dwHighDateTime << 32) | ft.dwLowDateTime; }
        constexpr float kGB = 1024.0f * 1024.0f * 1024.0f;
    }

    void Update()
    {
        FILETIME idle, kern, user;
        if (GetSystemTimes(&idle, &kern, &user))
        {
            const ULONGLONG i = ToU64(idle), t = ToU64(kern) + ToU64(user); // kernel time includes idle
            if (g_prevTotal != 0 && t > g_prevTotal)
            {
                const float usage = 1.0f - (float)(i - g_prevIdle) / (float)(t - g_prevTotal);
                g_snap.cpu = usage < 0.0f ? 0.0f : (usage > 1.0f ? 1.0f : usage);
            }
            g_prevIdle  = i;
            g_prevTotal = t;
        }

        MEMORYSTATUSEX ms = { sizeof(ms) };
        if (GlobalMemoryStatusEx(&ms))
        {
            g_snap.ramTotalGB = (float)ms.ullTotalPhys / kGB;
            g_snap.ramUsedGB  = (float)(ms.ullTotalPhys - ms.ullAvailPhys) / kGB;
            g_snap.ramFrac    = g_snap.ramTotalGB > 0 ? g_snap.ramUsedGB / g_snap.ramTotalGB : 0.0f;
        }

        ULARGE_INTEGER freeB, totalB, totalFree;
        if (GetDiskFreeSpaceExW(L"C:\\", &freeB, &totalB, &totalFree))
        {
            g_snap.diskFreeGB  = (float)freeB.QuadPart / kGB;
            g_snap.diskTotalGB = (float)totalB.QuadPart / kGB;
            g_snap.diskFrac    = g_snap.diskTotalGB > 0 ? 1.0f - g_snap.diskFreeGB / g_snap.diskTotalGB : 0.0f;
        }

        SYSTEM_INFO si;
        GetSystemInfo(&si);
        g_snap.threads   = (int)si.dwNumberOfProcessors;
        g_snap.uptimeSec = GetTickCount64() / 1000ULL;
    }

    const Snapshot& Get() { return g_snap; }

    std::string UserName()
    {
        char buf[256];
        DWORD n = sizeof(buf);
        return GetUserNameA(buf, &n) ? std::string(buf) : std::string("User");
    }

    std::string ComputerName()
    {
        char buf[MAX_COMPUTERNAME_LENGTH + 1];
        DWORD n = sizeof(buf);
        return GetComputerNameA(buf, &n) ? std::string(buf) : std::string("Unknown");
    }

    std::string OsName()
    {
        using RtlGetVersionFn = LONG(WINAPI*)(PRTL_OSVERSIONINFOW);
        RTL_OSVERSIONINFOW vi = { sizeof(vi) };
        if (HMODULE ntdll = GetModuleHandleW(L"ntdll.dll"))
            if (auto fn = (RtlGetVersionFn)(void*)GetProcAddress(ntdll, "RtlGetVersion"))
                fn(&vi);

        char buf[64];
        snprintf(buf, sizeof(buf), "Windows %s (build %lu)", vi.dwBuildNumber >= 22000 ? "11" : "10", vi.dwBuildNumber);
        return buf;
    }

    std::string CpuName()
    {
        static std::string cached;
        if (!cached.empty()) return cached;
        HKEY key;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &key) != ERROR_SUCCESS)
            return "Unknown";
        wchar_t buf[256] = {};
        DWORD sz = sizeof(buf);
        RegQueryValueExW(key, L"ProcessorNameString", nullptr, nullptr, (LPBYTE)buf, &sz);
        RegCloseKey(key);
        char out[256];
        WideCharToMultiByte(CP_UTF8, 0, buf, -1, out, sizeof(out), nullptr, nullptr);
        std::string name(out);
        size_t s = name.find_first_not_of(' ');
        size_t e = name.find_last_not_of(' ');
        cached = (s != std::string::npos) ? name.substr(s, e - s + 1) : name;
        return cached;
    }

    std::string GpuName()
    {
        static std::string cached;
        if (!cached.empty()) return cached;
        IDXGIFactory* factory = nullptr;
        if (FAILED(CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory)))
            return "Unknown";
        IDXGIAdapter* adapter = nullptr;
        if (FAILED(factory->EnumAdapters(0, &adapter)))
        { factory->Release(); return "Unknown"; }
        DXGI_ADAPTER_DESC desc;
        adapter->GetDesc(&desc);
        adapter->Release();
        factory->Release();
        char buf[128];
        WideCharToMultiByte(CP_UTF8, 0, desc.Description, -1, buf, sizeof(buf), nullptr, nullptr);
        cached = buf;
        return cached;
    }

    std::string Hwid()
    {
        static std::string cached;
        if (!cached.empty())
            return cached;

        DWORD serial = 0;
        GetVolumeInformationA("C:\\", nullptr, 0, &serial, nullptr, nullptr, nullptr, 0);
        const std::string seed = ComputerName() + "|" + std::to_string(serial);

        uint64_t h = 1469598103934665603ULL; // FNV-1a
        for (unsigned char c : seed)
        {
            h ^= c;
            h *= 1099511628211ULL;
        }

        char buf[32];
        snprintf(buf, sizeof(buf), "%04X-%04X-%04X-%04X",
                 (unsigned)(h >> 48) & 0xFFFF, (unsigned)(h >> 32) & 0xFFFF, (unsigned)(h >> 16) & 0xFFFF, (unsigned)h & 0xFFFF);
        cached = buf;
        return cached;
    }
}
