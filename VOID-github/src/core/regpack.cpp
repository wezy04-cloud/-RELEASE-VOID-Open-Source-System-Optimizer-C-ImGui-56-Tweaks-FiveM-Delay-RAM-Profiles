#include "regpack.hpp"
#include <windows.h>
#include <string>

namespace regpack
{
    namespace
    {
        const char* kHeader = "Windows Registry Editor Version 5.00\r\n\r\n";

        bool RunRegImport(const wchar_t* path)
        {
            // reg.exe reports a real exit code; regedit /s always returns 0.
            std::wstring cmd = L"reg.exe import \"";
            cmd += path;
            cmd += L"\"";

            STARTUPINFOW si = { sizeof(si) };
            si.dwFlags     = STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_HIDE;
            PROCESS_INFORMATION pi = {};

            std::wstring buf = cmd;
            if (!CreateProcessW(nullptr, &buf[0], nullptr, nullptr, FALSE,
                                CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi))
                return false;

            WaitForSingleObject(pi.hProcess, 20000);
            DWORD code = 1;
            GetExitCodeProcess(pi.hProcess, &code);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            return code == 0;
        }
    }

    bool Import(const char* text)
    {
        if (!text || !*text) return false;

        wchar_t dir[MAX_PATH];
        if (!GetTempPathW(MAX_PATH, dir)) return false;

        wchar_t path[MAX_PATH];
        swprintf_s(path, L"%svoid_%lu_%lu.reg", dir, GetCurrentProcessId(), GetTickCount());

        // reg.exe expects CRLF; the embedded bodies are plain LF
        std::string norm;
        norm.reserve(strlen(text) + 128);
        for (const char* p = text; *p; ++p)
        {
            if (*p == '\n' && (p == text || p[-1] != '\r')) norm += '\r';
            norm += *p;
        }

        const int wn = MultiByteToWideChar(CP_UTF8, 0, norm.c_str(), -1, nullptr, 0);
        if (wn <= 1) return false;
        std::wstring w((size_t)wn - 1, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, norm.c_str(), -1, &w[0], wn);

        HANDLE f = CreateFileW(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                               FILE_ATTRIBUTE_TEMPORARY, nullptr);
        if (f == INVALID_HANDLE_VALUE) return false;

        // "Version 5.00" .reg files are UTF-16LE and must start with a BOM
        const wchar_t bom = 0xFEFF;
        DWORD written = 0;
        BOOL ok = WriteFile(f, &bom, sizeof(bom), &written, nullptr);
        ok = ok && WriteFile(f, w.data(), (DWORD)(w.size() * sizeof(wchar_t)), &written, nullptr);
        CloseHandle(f);

        const bool imported = ok && RunRegImport(path);
        DeleteFileW(path);
        return imported;
    }

    bool ImportMany(const char* const* bodies, int count)
    {
        if (!bodies || count <= 0) return false;

        std::string all = kHeader;
        for (int i = 0; i < count; ++i)
        {
            if (!bodies[i]) continue;
            // each body carries its own header line; keep only the first one
            const char* keys = strchr(bodies[i], '\n');
            all += keys ? keys + 1 : bodies[i];
            all += "\n";
        }
        return Import(all.c_str());
    }

    // ------------------------------------------------------------------ Games

    const char* kGames[8][2] = {
        {   // 0 - game task scheduling priority
R"(Windows Registry Editor Version 5.00

[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Multimedia\SystemProfile\Tasks\Games]
"GPU Priority"=dword:00000008
"Priority"=dword:00000006
"Scheduling Category"="High"
"SFIO Priority"="High"
)",
R"(Windows Registry Editor Version 5.00

[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Multimedia\SystemProfile\Tasks\Games]
"GPU Priority"=dword:00000002
"Priority"=dword:00000002
"Scheduling Category"="Medium"
"SFIO Priority"="Normal"
)"  },
        {   // 1 - Game DVR / fullscreen optimizations
R"(Windows Registry Editor Version 5.00

[HKEY_CURRENT_USER\System\GameConfigStore]
"GameDVR_Enabled"=dword:00000000
"GameDVR_FSEBehaviorMode"=dword:00000002

[HKEY_LOCAL_MACHINE\SOFTWARE\Policies\Microsoft\Windows\GameDVR]
"AllowGameDVR"=dword:00000000
)",
R"(Windows Registry Editor Version 5.00

[HKEY_CURRENT_USER\System\GameConfigStore]
"GameDVR_Enabled"=dword:00000001
"GameDVR_FSEBehaviorMode"=dword:00000000

[HKEY_LOCAL_MACHINE\SOFTWARE\Policies\Microsoft\Windows\GameDVR]
"AllowGameDVR"=dword:00000001
)"  },
        {   // 2 - clock rate / affinity / foreground
R"(Windows Registry Editor Version 5.00

[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Multimedia\SystemProfile\Tasks\Games]
"Affinity"=dword:00000000
"Background Only"="False"
"Clock Rate"=dword:00002710
)",
R"(Windows Registry Editor Version 5.00

[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Multimedia\SystemProfile\Tasks\Games]
"Affinity"=dword:00000000
"Background Only"="True"
"Clock Rate"=dword:00002710
)"  },
        {   // 3 - per-CPU-core GPU DPC (GPU Tweaks.reg)
R"(Windows Registry Editor Version 5.00

[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\GraphicsDrivers]
"RmGpsPsEnablePerCpuCoreDpc"=dword:00000001

[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\GraphicsDrivers\Power]
"RmGpsPsEnablePerCpuCoreDpc"=dword:00000001

[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\nvlddmkm]
"RmGpsPsEnablePerCpuCoreDpc"=dword:00000001

[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\nvlddmkm\NVAPI]
"RmGpsPsEnablePerCpuCoreDpc"=dword:00000001

[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\nvlddmkm\Global\NVTweak]
"RmGpsPsEnablePerCpuCoreDpc"=dword:00000001
)",
R"(Windows Registry Editor Version 5.00

[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\GraphicsDrivers]
"RmGpsPsEnablePerCpuCoreDpc"=-

[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\GraphicsDrivers\Power]
"RmGpsPsEnablePerCpuCoreDpc"=-

[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\nvlddmkm]
"RmGpsPsEnablePerCpuCoreDpc"=-

[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\nvlddmkm\NVAPI]
"RmGpsPsEnablePerCpuCoreDpc"=-

[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\nvlddmkm\Global\NVTweak]
"RmGpsPsEnablePerCpuCoreDpc"=-
)"  },
        {   // 4 - NVIDIA driver thread priority
R"(Windows Registry Editor Version 5.00

[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\nvlddmkm\Parameters]
"ThreadPriority"=dword:0000001f
)",
R"(Windows Registry Editor Version 5.00

[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\nvlddmkm\Parameters]
"ThreadPriority"=-
)"  },
        {   // 5 - keep kernel code resident
R"(Windows Registry Editor Version 5.00

[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager\Memory Management]
"DisablePagingExecutive"=dword:00000001
)",
R"(Windows Registry Editor Version 5.00

[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager\Memory Management]
"DisablePagingExecutive"=dword:00000000
)"  },
        {   // 6 - large system cache
R"(Windows Registry Editor Version 5.00

[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager\Memory Management]
"LargeSystemCache"=dword:00000001
)",
R"(Windows Registry Editor Version 5.00

[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager\Memory Management]
"LargeSystemCache"=dword:00000000
)"  },
        {   // 7 - Memory Optimization.reg
R"(Windows Registry Editor Version 5.00

[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager\Memory Management]
"IoPageLockLimit"=dword:00100000
"PoolUsageMaximum"=dword:00000060
"SecondLevelDataCache"=dword:00000c00
"SessionPoolSize"=dword:000000c0
"SessionViewSize"=dword:000000c0
"PagedPoolSize"=dword:000000c0
"PagedPoolQuota"=dword:00000000
"NonPagedPoolQuota"=dword:00000000
"NonPagedPoolSize"=dword:00000000
"SystemPages"=dword:00000000
)",
R"(Windows Registry Editor Version 5.00

[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager\Memory Management]
"IoPageLockLimit"=-
"PoolUsageMaximum"=-
"SecondLevelDataCache"=-
"SessionPoolSize"=-
"SessionViewSize"=-
"PagedPoolSize"=-
"PagedPoolQuota"=-
)"  },
    };

    // ------------------------------------------------------------------ FiveM

    const char* kFiveM[8][2] = {
        {   // 0 - FiveM_Boost.reg
R"(Windows Registry Editor Version 5.00

[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Multimedia\SystemProfile\Tasks\Games]
"Affinity"=dword:00000000
"Background Only"="False"
"Clock Rate"=dword:00002710
"GPU Priority"=dword:00000008
"Priority"=dword:00000006
"Scheduling Category"="High"
"SFIO Priority"="High"
)",
R"(Windows Registry Editor Version 5.00

[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Multimedia\SystemProfile\Tasks\Games]
"Affinity"=dword:00000000
"Background Only"="True"
"Clock Rate"=dword:00002710
"GPU Priority"=dword:00000002
"Priority"=dword:00000002
"Scheduling Category"="Medium"
"SFIO Priority"="Normal"
)"  },
        {   // 1 - Otimizacao_de_Sistema.reg
R"(Windows Registry Editor Version 5.00

[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Multimedia\SystemProfile]
"SystemResponsiveness"=dword:00000000
)",
R"(Windows Registry Editor Version 5.00

[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Multimedia\SystemProfile]
"SystemResponsiveness"=dword:00000014
)"  },
        {   // 2 - network throttling off
R"(Windows Registry Editor Version 5.00

[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Multimedia\SystemProfile]
"NetworkThrottlingIndex"=dword:ffffffff
)",
R"(Windows Registry Editor Version 5.00

[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Multimedia\SystemProfile]
"NetworkThrottlingIndex"=dword:0000000a
)"  },
        {   // 3 - instant menus
R"(Windows Registry Editor Version 5.00

[HKEY_CURRENT_USER\Control Panel\Desktop]
"MenuShowDelay"="0"
)",
R"(Windows Registry Editor Version 5.00

[HKEY_CURRENT_USER\Control Panel\Desktop]
"MenuShowDelay"="400"
)"  },
        {   // 4 - Boost_Responsiveness.reg timeouts
R"(Windows Registry Editor Version 5.00

[HKEY_CURRENT_USER\Control Panel\Desktop]
"AutoEndTasks"="1"
"HungAppTimeout"="4000"
"WaitToKillAppTimeout"="5000"
)",
R"(Windows Registry Editor Version 5.00

[HKEY_CURRENT_USER\Control Panel\Desktop]
"AutoEndTasks"="0"
"HungAppTimeout"="5000"
"WaitToKillAppTimeout"="20000"
)"  },
        {   // 5 - low level hooks timeout
R"(Windows Registry Editor Version 5.00

[HKEY_CURRENT_USER\Control Panel\Desktop]
"LowLevelHooksTimeout"="1000"
)",
R"(Windows Registry Editor Version 5.00

[HKEY_CURRENT_USER\Control Panel\Desktop]
"LowLevelHooksTimeout"="5000"
)"  },
        {   // 6 - service shutdown timeout
R"(Windows Registry Editor Version 5.00

[HKEY_CURRENT_USER\Control Panel\Desktop]
"WaitToKillServiceTimeout"="1000"

[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control]
"WaitToKillServiceTimeout"="2000"
)",
R"(Windows Registry Editor Version 5.00

[HKEY_CURRENT_USER\Control Panel\Desktop]
"WaitToKillServiceTimeout"="5000"

[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control]
"WaitToKillServiceTimeout"="5000"
)"  },
        {   // 7 - lighter window movement
R"(Windows Registry Editor Version 5.00

[HKEY_CURRENT_USER\Control Panel\Desktop]
"DragFullWindows"="0"
"ForegroundLockTimeout"="150000"
)",
R"(Windows Registry Editor Version 5.00

[HKEY_CURRENT_USER\Control Panel\Desktop]
"DragFullWindows"="1"
"ForegroundLockTimeout"="200000"
)"  },
    };

    // ------------------------------------------------------------------ Delay

    const char* kDelay[8][2] = {
        {   // 0 - TIMERRES.reg
R"(Windows Registry Editor Version 5.00

[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager\kernel]
"GlobalTimerResolutionRequests"=dword:00000001
)",
R"(Windows Registry Editor Version 5.00

[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager\kernel]
"GlobalTimerResolutionRequests"=dword:00000000
)"  },
        {   // 1 - DPC watchdog profile
R"(Windows Registry Editor Version 5.00

[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager\kernel]
"DpcWatchdogProfileOffset"=dword:00000000
"SeTokenSingletonAttributesConfig"=dword:00000003
)",
R"(Windows Registry Editor Version 5.00

[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager\kernel]
"DpcWatchdogProfileOffset"=-
"SeTokenSingletonAttributesConfig"=-
)"  },
        {   // 2 - exception chain validation
R"(Windows Registry Editor Version 5.00

[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager\kernel]
"DisableExceptionChainValidation"=dword:00000001
"KernelSEHOPEnabled"=dword:00000000
)",
R"(Windows Registry Editor Version 5.00

[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager\kernel]
"DisableExceptionChainValidation"=dword:00000000
"KernelSEHOPEnabled"=dword:00000001
)"  },
        {   // 3 - interrupt steering
R"(Windows Registry Editor Version 5.00

[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager\kernel]
"InterruptSteeringDisabled"=dword:00000001
"obcaseinsensitive"=dword:00000001
)",
R"(Windows Registry Editor Version 5.00

[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager\kernel]
"InterruptSteeringDisabled"=dword:00000000
)"  },
        {   // 4 - ReduceInputDelay.reg
R"(Windows Registry Editor Version 5.00

[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\PriorityControl]
"Win32PrioritySeparation"=dword:00000028
)",
R"(Windows Registry Editor Version 5.00

[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\PriorityControl]
"Win32PrioritySeparation"=dword:00000002
)"  },
        {   // 5 - TimeStampInterval0ms.reg
R"(Windows Registry Editor Version 5.00

[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Reliability]
"TimeStampInterval"=dword:00000000
)",
R"(Windows Registry Editor Version 5.00

[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Reliability]
"TimeStampInterval"=dword:00000001
)"  },
        {   // 6 - Reduzir_Delay.reg
R"(Windows Registry Editor Version 5.00

[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\services\LanmanServer\Parameters]
"autodisconnect"=dword:ffffffff
"Size"=dword:00000003
"EnableOplocks"=dword:00000000
"IRPStackSize"=dword:00000020
"SharingViolationDelay"=dword:00000000
"SharingViolationRetries"=dword:00000000
)",
R"(Windows Registry Editor Version 5.00

[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\services\LanmanServer\Parameters]
"autodisconnect"=dword:0000000f
"Size"=dword:00000001
"EnableOplocks"=dword:00000001
"SharingViolationDelay"=-
"SharingViolationRetries"=-
)"  },
        {   // 7 - mouse-fix.reg + FIX_Input_Delay.reg
R"(Windows Registry Editor Version 5.00

[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\PrecisionTouchPad]
"AAPThreshold"=dword:0000000a

[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager\Memory Management]
"FeatureSettings"=dword:00000001
)",
R"(Windows Registry Editor Version 5.00

[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\PrecisionTouchPad]
"AAPThreshold"=dword:00000000

[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager\Memory Management]
"FeatureSettings"=dword:00000000
)"  },
    };

    const char* Body(int cat, int idx, bool enable)
    {
        if (idx < 0 || idx > 7) return nullptr;
        const int k = enable ? 0 : 1;
        switch (cat)
        {
        case 4: return kGames[idx][k];
        case 5: return kFiveM[idx][k];
        case 6: return kDelay[idx][k];
        }
        return nullptr;
    }

    bool ApplyRamProfile(int gb)
    {
        // SvcHostSplitThresholdInKB: installed RAM in KB, or 0x380000 for the default
        const unsigned threshold = gb > 0 ? (unsigned)gb * 1024u * 1024u : 0x380000u;

        char body[512];
        snprintf(body, sizeof(body),
                 "%s"
                 "[HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control]\r\n"
                 "\"SvcHostSplitThresholdInKB\"=dword:%08x\r\n"
                 "\r\n"
                 "[HKEY_LOCAL_MACHINE\\SYSTEM\\ControlSet001\\Control]\r\n"
                 "\"SvcHostSplitThresholdInKB\"=dword:%08x\r\n",
                 kHeader, threshold, threshold);
        return Import(body);
    }
}
