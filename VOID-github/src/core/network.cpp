#include "network.hpp"
#include <windows.h>
#include <iphlpapi.h>
#include <string>
#include <cstring>

#pragma comment(lib, "iphlpapi.lib")

namespace network
{
    namespace
    {
        bool RunCmd(const wchar_t* cmd)
        {
            STARTUPINFOW si = { sizeof(si) };
            si.dwFlags = STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_HIDE;
            PROCESS_INFORMATION pi = {};
            wchar_t buf[512];
            wcscpy_s(buf, cmd);
            if (!CreateProcessW(nullptr, buf, nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi))
                return false;
            WaitForSingleObject(pi.hProcess, 15000);
            DWORD code = 1;
            GetExitCodeProcess(pi.hProcess, &code);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            return code == 0;
        }

        DWORD RegGet(HKEY root, const wchar_t* path, const wchar_t* name, DWORD def)
        {
            HKEY k;
            if (RegOpenKeyExW(root, path, 0, KEY_READ, &k) != ERROR_SUCCESS) return def;
            DWORD val = def, sz = sizeof(val);
            RegQueryValueExW(k, name, nullptr, nullptr, (LPBYTE)&val, &sz);
            RegCloseKey(k);
            return val;
        }

        bool RegPut(HKEY root, const wchar_t* path, const wchar_t* name, DWORD val)
        {
            HKEY k;
            if (RegCreateKeyExW(root, path, 0, nullptr, 0, KEY_WRITE, nullptr, &k, nullptr) != ERROR_SUCCESS)
                return false;
            bool ok = RegSetValueExW(k, name, 0, REG_DWORD, (const BYTE*)&val, sizeof(val)) == ERROR_SUCCESS;
            RegCloseKey(k);
            return ok;
        }

        std::wstring FindAdapter()
        {
            ULONG sz = sizeof(IP_ADAPTER_INFO) * 16;
            IP_ADAPTER_INFO* info = (IP_ADAPTER_INFO*)malloc(sz);
            if (!info) return L"Ethernet";
            if (GetAdaptersInfo(info, &sz) == ERROR_BUFFER_OVERFLOW)
            {
                free(info);
                info = (IP_ADAPTER_INFO*)malloc(sz);
                if (!info) return L"Ethernet";
            }
            if (GetAdaptersInfo(info, &sz) != NO_ERROR) { free(info); return L"Ethernet"; }

            std::string guid;
            for (IP_ADAPTER_INFO* a = info; a; a = a->Next)
                if (strcmp(a->IpAddressList.IpAddress.String, "0.0.0.0") != 0)
                { guid = a->AdapterName; break; }
            free(info);

            if (guid.empty()) return L"Ethernet";

            std::wstring regPath = L"SYSTEM\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}\\";
            wchar_t guidW[256];
            MultiByteToWideChar(CP_ACP, 0, guid.c_str(), -1, guidW, 256);
            regPath += guidW;
            regPath += L"\\Connection";

            HKEY k;
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, regPath.c_str(), 0, KEY_READ, &k) != ERROR_SUCCESS)
                return L"Ethernet";
            wchar_t name[256] = {};
            DWORD nsz = sizeof(name);
            RegQueryValueExW(k, L"Name", nullptr, nullptr, (LPBYTE)name, &nsz);
            RegCloseKey(k);
            return name[0] ? name : L"Ethernet";
        }
    }

    bool FlushDns()
    {
        return RunCmd(L"ipconfig /flushdns");
    }

    bool SetDns(int provider)
    {
        std::wstring adapter = FindAdapter();
        std::wstring q = L"\"" + adapter + L"\"";

        if (provider == 0)
        {
            std::wstring cmd = L"netsh interface ipv4 set dns name=" + q + L" source=dhcp";
            return RunCmd(cmd.c_str());
        }

        const wchar_t* primary   = nullptr;
        const wchar_t* secondary = nullptr;
        switch (provider)
        {
        case 1: primary = L"1.1.1.1";   secondary = L"1.0.0.1";       break;
        case 2: primary = L"8.8.8.8";   secondary = L"8.8.4.4";       break;
        case 3: primary = L"9.9.9.9";   secondary = L"149.112.112.112"; break;
        default: return false;
        }

        std::wstring cmd1 = L"netsh interface ipv4 set dns name=" + q + L" static " + primary;
        std::wstring cmd2 = L"netsh interface ipv4 add dns name=" + q + L" " + secondary + L" index=2";
        bool ok = RunCmd(cmd1.c_str());
        RunCmd(cmd2.c_str());
        return ok;
    }

    bool ReadTweak(int idx)
    {
        switch (idx)
        {
        case 0: // TCP auto-tuning (on = normal/enabled)
            return true;
        case 1: // Nagle disabled
            return RegGet(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters",
                          L"TcpNoDelay", 0) == 1;
        case 2: // Network throttling disabled
            return RegGet(HKEY_LOCAL_MACHINE,
                          L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile",
                          L"NetworkThrottlingIndex", 10) == 0xFFFFFFFF;
        case 3: // QoS no bandwidth reserve
            return RegGet(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows\\Psched",
                          L"NonBestEffortLimit", 20) == 0;
        case 4: // LSO disabled
            return false;
        case 5: // Interrupt moderation disabled
            return false;
        }
        return false;
    }

    bool ApplyTweak(int idx, bool on)
    {
        switch (idx)
        {
        case 0:
            return on ? RunCmd(L"netsh int tcp set global autotuninglevel=normal")
                      : RunCmd(L"netsh int tcp set global autotuninglevel=disabled");
        case 1:
            return RegPut(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters",
                          L"TcpNoDelay", on ? 1 : 0) &&
                   RegPut(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters",
                          L"TcpAckFrequency", on ? 1 : 2);
        case 2:
            return RegPut(HKEY_LOCAL_MACHINE,
                          L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile",
                          L"NetworkThrottlingIndex", on ? 0xFFFFFFFF : 10);
        case 3:
            return RegPut(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows\\Psched",
                          L"NonBestEffortLimit", on ? 0 : 20);
        case 4:
            return on ? RunCmd(L"netsh int ip set global taskoffload=disabled")
                      : RunCmd(L"netsh int ip set global taskoffload=enabled");
        case 5:
            return false;
        }
        return false;
    }

    std::string AdapterName()
    {
        ULONG sz = sizeof(IP_ADAPTER_INFO) * 16;
        IP_ADAPTER_INFO* info = (IP_ADAPTER_INFO*)malloc(sz);
        if (!info) return "Unknown";
        if (GetAdaptersInfo(info, &sz) == ERROR_BUFFER_OVERFLOW)
        {
            free(info);
            info = (IP_ADAPTER_INFO*)malloc(sz);
            if (!info) return "Unknown";
        }
        if (GetAdaptersInfo(info, &sz) != NO_ERROR) { free(info); return "Unknown"; }
        std::string name;
        for (IP_ADAPTER_INFO* a = info; a; a = a->Next)
            if (strcmp(a->IpAddressList.IpAddress.String, "0.0.0.0") != 0)
            { name = a->Description; break; }
        free(info);
        return name.empty() ? "Unknown" : name;
    }

    std::string LocalIP()
    {
        ULONG sz = sizeof(IP_ADAPTER_INFO) * 16;
        IP_ADAPTER_INFO* info = (IP_ADAPTER_INFO*)malloc(sz);
        if (!info) return "";
        if (GetAdaptersInfo(info, &sz) == ERROR_BUFFER_OVERFLOW)
        {
            free(info);
            info = (IP_ADAPTER_INFO*)malloc(sz);
            if (!info) return "";
        }
        if (GetAdaptersInfo(info, &sz) != NO_ERROR) { free(info); return ""; }
        std::string ip;
        for (IP_ADAPTER_INFO* a = info; a; a = a->Next)
            if (strcmp(a->IpAddressList.IpAddress.String, "0.0.0.0") != 0)
            { ip = a->IpAddressList.IpAddress.String; break; }
        free(info);
        return ip;
    }

    std::string GatewayIP()
    {
        ULONG sz = sizeof(IP_ADAPTER_INFO) * 16;
        IP_ADAPTER_INFO* info = (IP_ADAPTER_INFO*)malloc(sz);
        if (!info) return "";
        if (GetAdaptersInfo(info, &sz) == ERROR_BUFFER_OVERFLOW)
        {
            free(info);
            info = (IP_ADAPTER_INFO*)malloc(sz);
            if (!info) return "";
        }
        if (GetAdaptersInfo(info, &sz) != NO_ERROR) { free(info); return ""; }
        std::string gw;
        for (IP_ADAPTER_INFO* a = info; a; a = a->Next)
            if (strcmp(a->GatewayList.IpAddress.String, "0.0.0.0") != 0)
            { gw = a->GatewayList.IpAddress.String; break; }
        free(info);
        return gw;
    }
}
