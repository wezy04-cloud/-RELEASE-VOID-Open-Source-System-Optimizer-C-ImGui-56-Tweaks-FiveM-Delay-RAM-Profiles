#include "ram.hpp"
#include "regpack.hpp"
#include <windows.h>

namespace ram
{
    namespace
    {
        struct Preset { int gb; const char* name; };

        // index 0 restores the Windows default (0x380000 KB = 3.5 GB)
        const Preset kPresets[] = {
            {   0, "Default" },
            {   4, "4 GB"    },
            {   6, "6 GB"    },
            {   8, "8 GB"    },
            {  12, "12 GB"   },
            {  16, "16 GB"   },
            {  24, "24 GB"   },
            {  32, "32 GB"   },
            {  64, "64 GB"   },
            {  96, "96 GB"   },
            { 128, "128 GB"  },
            { 192, "192 GB"  },
            { 256, "256 GB"  },
            { 512, "512 GB"  },
        };
        constexpr int kCount      = (int)(sizeof(kPresets) / sizeof(kPresets[0]));
        constexpr DWORD kDefaultKB = 0x380000;

        const wchar_t* kValue = L"SvcHostSplitThresholdInKB";

        DWORD ThresholdFor(int index)
        {
            if (index <= 0 || index >= kCount) return kDefaultKB;
            return (DWORD)kPresets[index].gb * 1024u * 1024u;
        }

        DWORD ReadThreshold()
        {
            HKEY k;
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control", 0, KEY_READ, &k) != ERROR_SUCCESS)
                return kDefaultKB;
            DWORD val = kDefaultKB, sz = sizeof(val);
            RegQueryValueExW(k, kValue, nullptr, nullptr, (LPBYTE)&val, &sz);
            RegCloseKey(k);
            return val;
        }
    }

    int         PresetCount()          { return kCount; }
    int         PresetGB(int i)        { return (i >= 0 && i < kCount) ? kPresets[i].gb : 0; }
    const char* PresetName(int i)      { return (i >= 0 && i < kCount) ? kPresets[i].name : "?"; }

    int DetectedGB()
    {
        MEMORYSTATUSEX ms = { sizeof(ms) };
        if (!GlobalMemoryStatusEx(&ms)) return 0;
        const double gb = (double)ms.ullTotalPhys / (1024.0 * 1024.0 * 1024.0);

        int best = 0;
        double bestDiff = 1e18;
        for (int i = 1; i < kCount; ++i)
        {
            const double diff = kPresets[i].gb - gb;
            // prefer the smallest preset that is >= installed RAM
            const double score = diff >= -0.6 ? diff : 1e9 - diff;
            if (score < bestDiff) { bestDiff = score; best = kPresets[i].gb; }
        }
        return best;
    }

    int CurrentPreset()
    {
        const DWORD cur = ReadThreshold();
        if (cur == kDefaultKB) return 0;
        for (int i = 1; i < kCount; ++i)
            if (ThresholdFor(i) == cur)
                return i;
        return -1;
    }

    bool Apply(int index)
    {
        return regpack::ApplyRamProfile(PresetGB(index));
    }
}
