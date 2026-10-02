# VOID — System Optimizer

A Windows system optimizer written from scratch in C++. No .NET, no Electron — a single ~1 MB native executable with a custom monochrome UI built on [Dear ImGui](https://github.com/ocornut/imgui) and DirectX 11.

Every registry key it writes is visible in the source. Build it yourself if you don't want to trust a binary.

> **Create a restore point before applying tweaks.** These change real registry values.

---

## Features

### Dashboard
Live CPU / RAM / disk usage from the Win32 API, a rolling 30-second CPU load graph, a system health score, and one-click optimize (trims the working set and flushes the DNS cache).

### Cleaner
Real asynchronous file scanning and deletion across 10 categories — temp files, browser cache (Chrome / Edge / Firefox / Opera / Brave), Windows Update cache, Recycle Bin, prefetch, system logs, thumbnail cache, crash dumps, shader cache, and Delivery Optimization.

### Tweaks — 7 categories, 56 toggles

| Category | Covers |
|---|---|
| **Performance** | Power plan, background apps, visual effects, SysMain, hibernation, system responsiveness, power throttling, memory management |
| **Gaming** | Game Mode, fullscreen optimizations, hardware GPU scheduling, Game Bar, raw mouse input, timer resolution, CPU priority, Nagle |
| **Privacy** | Telemetry, activity history, advertising ID, location, Cortana, feedback, tailored experiences, error reporting |
| **Visual** | Animations, transparency, classic context menu, file extensions, hidden files, taskbar search, lock screen tips, startup delay |
| **Games** | Game task scheduling priority, Game DVR, clock rate, per-CPU-core GPU DPC, NVIDIA driver thread priority, paging executive, large system cache, IO page lock limit |
| **FiveM** | Full game task boost profile, `SystemResponsiveness` 0, network throttling off, instant menus, fast app termination, low-level hooks timeout, service kill timeout, window drag |
| **Delay** | Global timer resolution, DPC watchdog offset, exception chain validation, interrupt steering, `Win32PrioritySeparation` 0x28, reliability timestamp, LanmanServer sharing-violation fix, mouse input delay fix |

Every toggle reads its **current** state from the registry at startup, so the UI reflects what is actually applied rather than a saved config file.

### RAM optimization profile
Tunes `SvcHostSplitThresholdInKB` so Windows groups services into fewer `svchost.exe` processes instead of spawning one per service. 14 presets (Default, 4 GB → 512 GB); the app detects installed RAM and marks the matching preset as recommended.

### Network
Latency and jitter graph, one-click DNS switching (Cloudflare / Google / Quad9 / DHCP via `netsh`), DNS cache flush, active adapter details, and 6 network tweaks — TCP auto-tuning, Nagle, network throttling index, QoS, LSO, interrupt moderation.

### System info
CPU name / physical cores / logical threads / base clock, GPU name / VRAM / driver / DirectX version, total RAM, motherboard model, BIOS version, **BIOS mode (UEFI or Legacy)**, **Secure Boot status**, **virtualization status**, **Windows install date**, display resolution, and system locale.

### UI
Monochrome theme, borderless window with rounded corners, an interactive particle background whose particles react to the cursor, a top-down light glow and animated sweep. All effects are toggleable, and particle count and speed are adjustable. **English and Turkish** language support, with DPI-aware scaling throughout.

---

## How the tweaks are applied

The Games, FiveM and Delay categories ship as **`.reg` bodies embedded directly in `src/core/regpack.cpp`**. Nothing is downloaded and no external folder is required. At runtime VOID:

1. Writes the `.reg` text to `%TEMP%\void_<pid>_<tick>.reg` as UTF-16LE with a BOM
2. Imports it with `reg.exe import` in a hidden window
3. Deletes the temp file immediately

`reg.exe` is used rather than `regedit /s` on purpose: regedit returns exit code 0 even when an import fails, so a tweak could silently do nothing. `reg import` returns a real exit code, which is what lets the app honestly report that a tweak needs elevation.

Applying a whole category merges its bodies into a single `.reg` file, so it costs one import pass instead of eight separate processes. Every toggle has both an apply and a revert body — turning a tweak off restores the previous value or deletes the key.

---

## Building

```bat
build.bat
```

The script locates your Visual Studio installation and clones Dear ImGui into `third_party/` if it is missing. Output: `build\VOID.exe`.

CMake also works:

```bat
cmake -B build
cmake --build build --config Release
```

**Requirements:** Windows 10/11 x64, Visual Studio with the *Desktop development with C++* workload.

---

## Project layout

```
src/main.cpp               Borderless Win32 window, D3D11 device, rounded corners
src/app.cpp                Screens: Login -> Loading -> Dashboard / Cleaner / Tweaks /
                           Network / System Info / Settings
src/gui/theme.cpp          Colors, Segoe UI fonts, DPI scaling
src/gui/fx.cpp             Particles, constellation lines, top glow, light sweep
src/gui/widgets.cpp        Animated button, switch, checkbox, input, slider, segmented,
                           progress, ring, graph, toast
src/gui/icons.cpp          Vector icons (no icon font needed)
src/core/cleaner.cpp       Real file scanning and deletion
src/core/tweaks.cpp        Registry read/write for all 7 tweak categories
src/core/regpack.cpp       Embedded .reg bodies + temp-file import via reg.exe
src/core/ram.cpp           SvcHostSplitThresholdInKB profiles
src/core/network.cpp       DNS switching, adapter info, network tweaks
src/core/sysinfo.cpp       CPU / RAM / disk / uptime / HWID (read-only)
src/core/sysinfo_detail.cpp  Secure Boot, virtualization, BIOS mode, install date
src/core/lang.cpp          English / Turkish string tables
src/core/license.cpp       Demo license screen
```

---

## Notes

- **Run as administrator** for the Games, Delay and RAM tweaks — they write to `HKLM`. Without elevation the app reports the failure instead of silently doing nothing.
- Some tweaks require a restart to take effect.
- The license screen is a **demo**: any key is accepted. There is no server, no activation check and no telemetry — see `src/core/license.cpp`.
- The registry values are drawn from well-known community `.reg` packs (Trimors, EverythingTech and others). Credit to their authors — this project wraps them in a proper UI and makes them reversible.

## License

[MIT](LICENSE)
