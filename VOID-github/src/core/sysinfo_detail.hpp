#pragma once
#include <string>

namespace sysdetail
{
    struct Info
    {
        int    cpuCores       = 0;
        int    cpuThreads     = 0;
        std::string cpuClock;

        std::string gpuVram;
        std::string gpuDriver;

        std::string ramTotal;
        std::string ramSpeed;
        std::string ramSlots;

        std::string motherboard;
        std::string biosVersion;
        std::string biosMode;

        std::string secureBoot;
        std::string virtualization;
        std::string installDate;
        std::string directX;
        std::string displayRes;
        std::string systemLocale;
    };

    void        Gather();
    const Info& Get();
}
