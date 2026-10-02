#pragma once

namespace lang
{
    enum Id
    {
        EN = 0,
        TR = 1,
    };

    void        Set(Id id);
    Id          Current();
    const char* Get(int key);
}

namespace S
{
    enum Key
    {
        SecureLoader, PremiumSystemOptimizer, LicenseKey, Generate, RememberMe,
        ActivateLicense, DemoMode, AnyKeyAccepted, ContactingServer,
        PleaseEnterKey, ActivationFailed, SignedOut, SessionClosed,

        Dashboard, Cleaner, Tweaks, Network, Settings, SystemInfo,
        DashOverview, CleanDesc, TweakDesc, NetDesc, SettDesc, SysInfoDesc,

        Processor, Memory, Storage, Health, Excellent, Good, NeedsAttention,
        LogicalThreads, OfGBInUse, FreeOfGB,
        ProcessorLoad, LiveLast30s, QuickOptimize, OneClickEvery,
        HealthScore, Optimizing, OptimizeNow,
        SystemOptimized, MemTrimmed,

        System, Subscription, OperatingSystem, Graphics, Computer, User, Uptime,
        Plan, Status, Active, Expires, HWID,

        ReadyToScan, SelectCategories, ScanComplete, JunkFound, ReadyToClean,
        AllClean, Spotless, Freed, Scan, Rescan, CleanNow, Categories,

        TempFiles, BrowserCache, WinUpdateCache, RecycleBin, PrefetchData,
        SystemLogs, ThumbnailCache, CrashDumps, ShaderCache, DeliveryOpt,
        TempFilesDesc, BrowserCacheDesc, WinUpdateCacheDesc, RecycleBinDesc,
        PrefetchDesc, SystemLogsDesc, ThumbnailDesc, CrashDumpsDesc,
        ShaderCacheDesc, DeliveryOptDesc,

        Performance, Gaming, Privacy, Visual, Games, FiveM, Delay,
        Apply, Reset, DefaultsRestored, TweaksApplied, TweaksActive,
        NeedAdmin, RestartRecommended,

        RamOptimization, RamOptDesc, InstalledRam, Detected, CurrentProfile,
        ApplyRamProfile, RamProfileApplied, RamRestartNote, Custom, Recommended,

        Latency, RoundTrip, Simulated, Jitter,
        DnsProvider, ResolverUsed, FlushDnsCache, DnsUpdated, DnsFlushed,
        Connection, ActiveAdapter,
        Automatic, Optimizations,

        VisualEffects, TuneBg, Particles, ConnectionLines, MouseInteraction,
        TopLight, LightSweep, ParticleCount, ParticleSpeed,
        General, AppBehaviour, RememberLicense, Notifications, ShowToast,
        LaunchStartup, MinimizeToTray, Account, SignOut,
        Language,

        HardwareInfo, SoftwareInfo, SecurityInfo, NetworkInfo,
        CPUName, CPUCores, CPUThreads, CPUClock,
        GPUName, GPUVRAM, GPUDriver,
        RAMTotal, RAMSpeed, RAMSlots,
        Motherboard, BIOSVersion, BIOSMode,
        SecureBoot, Enabled, Disabled, NotSupported,
        Virtualization, InstallDate, DirectXVersion,
        DisplayResolution, SystemLocale,

        _COUNT
    };
}

#define L(k) lang::Get(S::k)
