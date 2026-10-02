#pragma once
#include <string>

// Read-only system information used by the dashboard.
namespace sys
{
    struct Snapshot
    {
        float cpu         = 0.0f; // 0..1
        float ramUsedGB   = 0.0f;
        float ramTotalGB  = 0.0f;
        float ramFrac     = 0.0f;
        float diskFreeGB  = 0.0f;
        float diskTotalGB = 0.0f;
        float diskFrac    = 0.0f; // used fraction
        int   threads     = 0;
        unsigned long long uptimeSec = 0;
    };

    void            Update();
    const Snapshot& Get();

    std::string UserName();
    std::string ComputerName();
    std::string OsName();
    std::string CpuName();
    std::string GpuName();
    std::string Hwid();
}
