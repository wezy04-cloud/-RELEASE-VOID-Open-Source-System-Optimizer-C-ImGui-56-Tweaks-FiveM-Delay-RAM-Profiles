#include "app.hpp"
#include "gui/theme.hpp"
#include "gui/widgets.hpp"
#include "gui/fx.hpp"
#include "gui/icons.hpp"
#include "core/license.hpp"
#include "core/sysinfo.hpp"
#include "core/cleaner.hpp"
#include "core/tweaks.hpp"
#include "core/network.hpp"
#include "core/lang.hpp"
#include "core/sysinfo_detail.hpp"
#include "core/ram.hpp"
#include "imgui_internal.h"
#include <string>
#include <vector>
#include <future>
#include <random>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <algorithm>

using theme::px;
using theme::White;
using theme::Gray;
using ui::ButtonStyle;
using ui::Toast;

namespace app
{
    namespace
    {
        constexpr const char* kVersion = "2.4.0";

        enum class Screen     { Login, Loading, Main };
        enum class CleanState { Idle, Scanning, Ready, Cleaning, Done };

        struct Category
        {
            const char* name;
            const char* desc;
            float       minMB, maxMB;
            bool        enabled;
            float       found  = 0.0f;
            float       target = 0.0f;
        };

        struct Tweak
        {
            const char* name;
            const char* desc;
            bool        on;
        };

        // ------------------------------------------------------------------ state

        HWND   g_hwnd   = nullptr;
        float  g_corner = 0.0f;

        Screen g_screen      = Screen::Login;
        Screen g_next        = Screen::Login;
        bool   g_switching   = false;
        double g_screenStart = 0.0;
        bool   g_closing     = false;
        bool   g_quit        = false;

        bool   g_dragging = false;
        POINT  g_dragStart{};
        RECT   g_dragRect{};

        char        g_key[64] = {};
        bool        g_reveal   = false;
        bool        g_remember = true;
        bool        g_waiting  = false;
        std::string g_loginError;
        double      g_errorTime = -100.0;

        int    g_tab     = 0;
        double g_tabTime = 0.0;

        std::vector<float> g_cpu(61, 0.0f);
        std::vector<float> g_ping(61, 18.0f);
        double g_lastSample = -1.0;
        float  g_health     = 78.0f;
        bool   g_optimizing = false;
        double g_optStart   = 0.0;

        struct CatKey { S::Key nameKey, descKey; };
        const CatKey kCatKeys[] = {
            { S::TempFiles,      S::TempFilesDesc },
            { S::BrowserCache,   S::BrowserCacheDesc },
            { S::WinUpdateCache, S::WinUpdateCacheDesc },
            { S::RecycleBin,     S::RecycleBinDesc },
            { S::PrefetchData,   S::PrefetchDesc },
            { S::SystemLogs,     S::SystemLogsDesc },
            { S::ThumbnailCache, S::ThumbnailDesc },
            { S::CrashDumps,     S::CrashDumpsDesc },
            { S::ShaderCache,    S::ShaderCacheDesc },
            { S::DeliveryOpt,    S::DeliveryOptDesc },
        };

        std::vector<Category> g_cats = {
            { "Temporary files",       "User and system temp folders",     180.0f,  900.0f, true  },
            { "Browser cache",         "Chrome, Edge, Firefox, Opera",     150.0f, 1200.0f, true  },
            { "Windows Update cache",  "Downloaded update packages",       300.0f, 2400.0f, true  },
            { "Recycle Bin",           "Deleted files waiting to go",        0.0f, 1800.0f, false },
            { "Prefetch data",         "Application launch traces",         20.0f,   90.0f, true  },
            { "System logs",           "Event and setup log files",         20.0f,  220.0f, true  },
            { "Thumbnail cache",       "Explorer preview database",         30.0f,  320.0f, true  },
            { "Crash dumps",           "Memory dumps and error reports",     0.0f,  800.0f, true  },
            { "Shader cache",          "DirectX and driver shaders",       100.0f,  650.0f, false },
            { "Delivery Optimization", "Peer-to-peer update cache",          0.0f, 1500.0f, true  },
        };
        CleanState g_cleanState = CleanState::Idle;
        double     g_cleanStart = 0.0;
        float      g_cleanP     = 0.0f;
        float      g_freed      = 0.0f;
        std::future<std::vector<cleaner::ScanResult>> g_asyncScan;
        std::future<double> g_asyncClean;
        bool g_scanDone  = false;
        bool g_cleanDone = false;

        constexpr int kTweakCats = 7;
        std::vector<Tweak> g_tweaks[kTweakCats] = {
            {   // Performance
                { "Ultimate power plan",       "Unlock the hidden power scheme",     true  },
                { "Disable background apps",   "Stop UWP apps running idle",         true  },
                { "Optimize visual effects",   "Keep smoothing, drop the rest",      false },
                { "Disable SysMain",           "Stop Superfetch disk thrashing",     false },
                { "Disable hibernation",       "Free hiberfil.sys disk space",       false },
                { "System responsiveness",     "Prioritize foreground tasks",        true  },
                { "Disable power throttling",  "Keep cores at full clock",           false },
                { "Memory management",         "Tune paging and cache sizes",        false },
            },
            {   // Gaming
                { "Game Mode",                 "Prioritize games for resources",     true  },
                { "Fullscreen optimizations",  "Disable for exclusive fullscreen",   false },
                { "Hardware GPU scheduling",   "Lower latency frame scheduling",     true  },
                { "Disable Xbox Game Bar",     "Remove overlay and DVR capture",     false },
                { "Raw mouse input",           "Turn off pointer acceleration",      true  },
                { "Timer resolution 0.5 ms",   "Tighter frame pacing",               false },
                { "High CPU priority",         "Boost the active game process",      false },
                { "Disable Nagle's algorithm", "Send packets immediately",           false },
            },
            {   // Privacy
                { "Disable telemetry",         "Stop diagnostic data uploads",       true  },
                { "Disable activity history",  "Do not log your timeline",           true  },
                { "Disable advertising ID",    "No personalized ad tracking",        true  },
                { "Disable location tracking", "Block system-wide location",         false },
                { "Disable Cortana",           "Turn off the voice assistant",       false },
                { "Block feedback requests",   "Never ask for feedback",             true  },
                { "Tailored experiences",      "Disable usage-based tips",           false },
                { "Disable error reporting",   "Do not send crash reports",          false },
            },
            {   // Visual
                { "Disable animations",        "Instant window transitions",         false },
                { "Disable transparency",      "Solid taskbar and menus",            false },
                { "Classic context menu",      "Full right-click menu",              true  },
                { "Show file extensions",      "Always show .exe, .txt, ...",        true  },
                { "Show hidden files",         "Reveal hidden folders",              false },
                { "Hide taskbar search",       "Reclaim taskbar space",              false },
                { "Disable lock screen tips",  "Clean lock screen",                  true  },
                { "Disable startup delay",     "Launch startup apps instantly",      false },
            },
            {   // Games
                { "Game task priority",        "GPU 8 / CPU 6 / high scheduling",    false },
                { "Disable Game DVR",          "Turn off recording and FSE mode",    false },
                { "Games clock rate",          "10000 ticks, no background cap",     false },
                { "Per-core GPU DPC",          "Spread driver DPCs across cores",    false },
                { "NVIDIA thread priority",    "Raise nvlddmkm to priority 31",      false },
                { "Disable paging executive",  "Keep kernel code in physical RAM",   false },
                { "Large system cache",        "Favour file cache over working set", false },
                { "IO page lock limit",        "1 MB lock limit, bigger L2 hint",    false },
            },
            {   // FiveM
                { "FiveM task boost",          "Full game task profile for FiveM",   false },
                { "System responsiveness 0",   "Give 100% of CPU to foreground",     false },
                { "Network throttling off",    "Remove the 10 packet/ms cap",        false },
                { "Menu show delay 0",         "Instant menus, no fade-in wait",     false },
                { "Fast app termination",      "Shorter hung-app timeouts",          false },
                { "Low level hooks timeout",   "1000 ms instead of 5000 ms",         false },
                { "Service kill timeout",      "Shut services down faster",          false },
                { "Disable full window drag",  "Lighter window movement",            false },
            },
            {   // Delay
                { "Global timer resolution",   "Honour 0.5 ms timer requests",       false },
                { "DPC watchdog offset",       "Relax the DPC watchdog profile",     false },
                { "Exception chain validation","Skip SEHOP chain checks",            false },
                { "Disable interrupt steering","Keep interrupts on their core",      false },
                { "Win32 priority separation", "0x28 - short, fixed quantums",       false },
                { "Reliability timestamp 0",   "Stop reliability sampling writes",   false },
                { "LanmanServer delay fix",    "No sharing-violation wait",          false },
                { "Mouse input delay fix",     "AAP threshold + feature settings",   false },
            },
        };
        int    g_tweakCat     = 0;
        double g_tweakCatTime = 0.0;
        bool   g_applying     = false;
        double g_applyStart   = 0.0;

        std::vector<Tweak> g_netTweaks = {
            { "TCP auto-tuning",           "Optimal receive window scaling",      true  },
            { "Disable Nagle's algorithm", "Lower latency for small packets",     false },
            { "Network throttling index",  "Remove multimedia throttling",        true  },
            { "QoS packet scheduler",      "Reserve no bandwidth for QoS",        false },
            { "Large send offload",        "Disable LSO on adapters",             false },
            { "Interrupt moderation",      "Faster packet processing",            false },
        };
        int g_dns = 1;

        bool g_startup = false;
        bool g_tray    = true;

        constexpr int kTabCount = 6;
        const Icon kTabIcons[] = { Icon::Dashboard, Icon::Cleaner, Icon::Tweaks, Icon::Network, Icon::Monitor, Icon::Settings };
        const S::Key kTabNameKeys[] = { S::Dashboard, S::Cleaner, S::Tweaks, S::Network, S::SystemInfo, S::Settings };
        const S::Key kTabSubKeys[]  = { S::DashOverview, S::CleanDesc, S::TweakDesc, S::NetDesc, S::SysInfoDesc, S::SettDesc };

        // ------------------------------------------------------------------ helpers

        float Rand(float a, float b)
        {
            static std::mt19937 rng{ std::random_device{}() };
            return std::uniform_real_distribution<float>(a, b)(rng);
        }

        std::string FormatSize(float mb)
        {
            char b[32];
            if (mb >= 1024.0f) snprintf(b, sizeof(b), "%.2f GB", mb / 1024.0f);
            else               snprintf(b, sizeof(b), "%.0f MB", mb);
            return b;
        }

        float TotalFound()
        {
            float t = 0.0f;
            for (const Category& c : g_cats)
                if (c.enabled)
                    t += c.found;
            return t;
        }

        void GoTo(Screen s)
        {
            if (g_switching || s == g_screen)
                return;
            g_next      = s;
            g_switching = true;
        }

        void TextCentered(ImDrawList* dl, ImFont* f, float size, float cx, float y, ImU32 col, const char* text)
        {
            const ImVec2 ts = ui::TextSize(f, size, text);
            ui::Text(dl, f, size, ImVec2(cx - ts.x * 0.5f, y), col, text);
        }

        void DrawLogo(ImDrawList* dl, const ImVec2& c, float s, float glow = 1.0f)
        {
            const float t = (float)ImGui::GetTime();
            const float h = s * 0.5f;
            fx::RadialGradient(dl, c, s * 1.5f, s * 1.5f, White(0.10f * glow), White(0.0f), 40);

            const ImVec2 d[4] = { ImVec2(c.x, c.y - h), ImVec2(c.x + h, c.y), ImVec2(c.x, c.y + h), ImVec2(c.x - h, c.y) };
            dl->AddPolyline(d, 4, White(0.95f), ImDrawFlags_Closed, ImMax(1.5f, s * 0.06f));

            const float a = t * 0.8f, r = h * 0.40f;
            ImVec2 q[4];
            for (int i = 0; i < 4; ++i)
                q[i] = ImVec2(c.x + cosf(a + i * IM_PI * 0.5f) * r, c.y + sinf(a + i * IM_PI * 0.5f) * r);
            dl->AddConvexPolyFilled(q, 4, White(0.95f));
        }

        void InfoRow(const char* k, const char* v, float w, bool sep = true)
        {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            const ImVec2 p = ImGui::GetCursorScreenPos();
            const float  h = px(30);
            const ImVec2 ks = ui::TextSize(theme::fonts.regular, 13.0f, k);
            const ImVec2 vs = ui::TextSize(theme::fonts.medium, 13.0f, v);
            ui::Text(dl, theme::fonts.regular, 13.0f, ImVec2(p.x, p.y + (h - ks.y) * 0.5f), Gray(0.50f), k);
            ui::Text(dl, theme::fonts.medium, 13.0f, ImVec2(p.x + w - vs.x, p.y + (h - vs.y) * 0.5f), Gray(0.92f), v);
            if (sep)
                dl->AddLine(ImVec2(p.x, p.y + h + px(3)), ImVec2(p.x + w, p.y + h + px(3)), White(0.05f));
            ImGui::Dummy(ImVec2(w, h));
        }

        // ------------------------------------------------------------------ actions

        void TryActivate()
        {
            if (g_waiting)
                return;
            if (!g_key[0])
            {
                g_loginError = "Please enter your license key";
                g_errorTime  = ImGui::GetTime();
                return;
            }
            g_loginError.clear();
            license::Activate(g_key);
            g_waiting = true;
        }

        void SignOut()
        {
            license::Logout();
            if (!g_remember)
            {
                license::ForgetSaved();
                memset(g_key, 0, sizeof(g_key));
            }
            g_loginError.clear();
            ui::Notify(Toast::Info, "Signed out", "Your session has been closed");
            GoTo(Screen::Login);
        }

        void GenerateKey()
        {
            static const char hex[] = "0123456789ABCDEF";
            char key[20];
            for (int i = 0; i < 19; ++i)
                key[i] = (i == 4 || i == 9 || i == 14) ? '-' : hex[(int)Rand(0, 15.99f)];
            key[19] = 0;
            strncpy_s(g_key, key, _TRUNCATE);
        }

        void StartScan()
        {
            for (Category& c : g_cats) { c.target = 0.0f; c.found = 0.0f; }
            g_cleanState = CleanState::Scanning;
            g_cleanStart = ImGui::GetTime();
            g_scanDone   = false;
            std::vector<bool> enabled;
            for (const Category& c : g_cats) enabled.push_back(c.enabled);
            g_asyncScan = std::async(std::launch::async, [enabled]() {
                std::vector<cleaner::ScanResult> results;
                for (int i = 0; i < (int)enabled.size(); ++i)
                    results.push_back(enabled[i] ? cleaner::Scan(i) : cleaner::ScanResult{});
                return results;
            });
        }

        void StartClean()
        {
            g_freed = 0.0f;
            for (Category& c : g_cats)
            {
                c.target = c.enabled ? c.found : 0.0f;
                g_freed += c.target;
            }
            g_cleanState = CleanState::Cleaning;
            g_cleanStart = ImGui::GetTime();
            g_cleanDone  = false;
            std::vector<bool> cats;
            for (const Category& c : g_cats) cats.push_back(c.enabled && c.found > 0.5f);
            g_asyncClean = std::async(std::launch::async, [cats]() {
                double total = 0.0;
                for (int i = 0; i < (int)cats.size(); ++i)
                    if (cats[i]) total += cleaner::Clean(i);
                return total;
            });
        }

        // ------------------------------------------------------------------ simulation / polling

        void UpdateData(double now)
        {
            if (g_lastSample < 0.0 || now - g_lastSample >= 0.5)
            {
                sys::Update();
                static bool primed = false; // first real CPU sample fills the history so the graph doesn't start flat
                if (!primed && sys::Get().cpu > 0.0f)
                {
                    std::fill(g_cpu.begin(), g_cpu.end(), sys::Get().cpu);
                    primed = true;
                }
                g_cpu.erase(g_cpu.begin());
                g_cpu.push_back(sys::Get().cpu);

                float nv = g_ping.back() + Rand(-2.5f, 2.5f);
                if (Rand(0.0f, 1.0f) < 0.05f)
                    nv += Rand(8.0f, 20.0f);
                nv = ImClamp(nv * 0.85f + 17.0f * 0.15f, 8.0f, 60.0f);
                g_ping.erase(g_ping.begin());
                g_ping.push_back(nv);
                g_lastSample = now;
            }

            if (g_waiting)
            {
                const license::Status st = license::Poll();
                if (st == license::Status::Valid)
                {
                    g_waiting = false;
                    if (g_remember) license::Save(license::Current().key);
                    else            license::ForgetSaved();
                    const std::string msg = license::Current().plan + " plan - welcome back!";
                    ui::Notify(Toast::Success, "License activated", msg.c_str());
                    GoTo(Screen::Loading);
                }
                else if (st == license::Status::Invalid)
                {
                    g_waiting    = false;
                    g_loginError = license::Error();
                    g_errorTime  = now;
                    ui::Notify(Toast::Error, "Activation failed", g_loginError.c_str());
                }
            }

            if (g_optimizing && now - g_optStart > 3.2)
            {
                g_optimizing = false;
                g_health     = Rand(96.0f, 99.0f);
                SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);
                network::FlushDns();
                ui::Notify(Toast::Success, "System optimized", "Memory trimmed, caches flushed");
            }

            const int n = (int)g_cats.size();
            if (g_cleanState == CleanState::Scanning)
            {
                g_cleanP = ImSaturate((float)(now - g_cleanStart) / 3.0f);
                if (!g_scanDone && g_asyncScan.valid() &&
                    g_asyncScan.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
                {
                    auto results = g_asyncScan.get();
                    for (int i = 0; i < n && i < (int)results.size(); ++i)
                        g_cats[i].target = (float)results[i].sizeMB;
                    g_scanDone = true;
                }
                for (int i = 0; i < n; ++i)
                {
                    const float k = ImSaturate(g_cleanP * n - (float)i);
                    g_cats[i].found = g_cats[i].target * k;
                }
                if (g_cleanP >= 1.0f && g_scanDone)
                {
                    g_cleanState = CleanState::Ready;
                    const std::string msg = FormatSize(TotalFound()) + " of junk found";
                    ui::Notify(Toast::Info, "Scan complete", msg.c_str());
                }
            }
            else if (g_cleanState == CleanState::Cleaning)
            {
                g_cleanP = ImSaturate((float)(now - g_cleanStart) / 2.4f);
                for (int i = 0; i < n; ++i)
                    if (g_cats[i].enabled)
                        g_cats[i].found = g_cats[i].target * (1.0f - ImSaturate(g_cleanP * n - (float)i));
                if (!g_cleanDone && g_asyncClean.valid() &&
                    g_asyncClean.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
                {
                    g_freed     = (float)g_asyncClean.get();
                    g_cleanDone = true;
                }
                if (g_cleanP >= 1.0f && g_cleanDone)
                {
                    g_cleanState = CleanState::Done;
                    g_health     = ImMin(99.0f, g_health + 9.0f);
                    const std::string msg = FormatSize(g_freed) + " freed successfully";
                    ui::Notify(Toast::Success, "Cleaning complete", msg.c_str());
                }
            }

            if (g_applying && now - g_applyStart > 1.4)
            {
                g_applying = false;
                int on = 0, fail = 0;
                for (int c = 0; c < kTweakCats; ++c)
                {
                    const int n = (int)g_tweaks[c].size();
                    for (int i = 0; i < n; ++i)
                        on += g_tweaks[c][i].on ? 1 : 0;

                    if (tweaks::IsRegPack(c))
                    {
                        bool states[16] = {};
                        for (int i = 0; i < n && i < 16; ++i)
                            states[i] = g_tweaks[c][i].on;
                        if (!tweaks::ApplyCategory(c, states, ImMin(n, 16))) fail += n;
                    }
                    else
                    {
                        for (int i = 0; i < n; ++i)
                            if (!tweaks::Apply(c, i, g_tweaks[c][i].on)) fail++;
                    }
                }
                char msg[128];
                if (fail > 0)
                    snprintf(msg, sizeof(msg), "%d tweaks active (%d need admin) - restart recommended", on, fail);
                else
                    snprintf(msg, sizeof(msg), "%d tweaks active - restart recommended", on);
                ui::Notify(fail > 0 ? Toast::Warning : Toast::Success, "Tweaks applied", msg);
            }
        }

        // ------------------------------------------------------------------ chrome

        void DrawWindowControls(const ImVec2& ds)
        {
            const ImVec2 bs = px(34, 28);
            const float  y  = px(12);
            ImGui::SetCursorScreenPos(ImVec2(ds.x - px(12) - bs.x * 2.0f - px(4), y));
            if (ui::IconButton("##minimize", Icon::Minimize, bs, px(12)))
                ShowWindow(g_hwnd, SW_MINIMIZE);
            ImGui::SetCursorScreenPos(ImVec2(ds.x - px(12) - bs.x, y));
            if (ui::IconButton("##close", Icon::Close, bs, px(11), true))
                g_closing = true;
        }

        void HandleDrag()
        {
            const ImGuiIO& io = ImGui::GetIO();
            const float dragH = g_screen == Screen::Main ? px(78) : px(60);
            if (ImGui::IsMouseClicked(0) && io.MousePos.y >= 0.0f && io.MousePos.y < dragH &&
                !ImGui::IsAnyItemHovered() && !ImGui::IsAnyItemActive())
            {
                g_dragging = true;
                GetCursorPos(&g_dragStart);
                GetWindowRect(g_hwnd, &g_dragRect);
            }
            if (g_dragging)
            {
                if (!ImGui::IsMouseDown(0))
                {
                    g_dragging = false;
                }
                else
                {
                    POINT p;
                    GetCursorPos(&p);
                    SetWindowPos(g_hwnd, nullptr, g_dragRect.left + p.x - g_dragStart.x, g_dragRect.top + p.y - g_dragStart.y,
                                 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
                }
            }
        }

        // ------------------------------------------------------------------ login

        void DrawLogin(const ImVec2& ds, float slide)
        {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            const double now = ImGui::GetTime();
            const auto& F = theme::fonts;

            // top-left brand
            ui::TextSpaced(dl, F.bold, 12.5f, px(22, 19), Gray(0.9f), "VOID", px(2.0f));
            const float bw = ui::SpacedSize(F.bold, 12.5f, "VOID", px(2.0f)).x;
            ui::Text(dl, F.regular, 12.5f, ImVec2(px(22) + bw + px(10), px(19)), Gray(0.38f), L(SecureLoader));

            // card (shakes on error)
            const float cw = px(380), ch = px(468);
            const float since = (float)(now - g_errorTime);
            const float shake = since < 0.5f ? sinf(since * 55.0f) * px(7) * (1.0f - since / 0.5f) : 0.0f;
            const ImVec2 cmin(floorf((ds.x - cw) * 0.5f + shake), floorf((ds.y - ch) * 0.5f + px(10) + slide));
            const ImVec2 cmax = cmin + ImVec2(cw, ch);
            const float  cx   = (cmin.x + cmax.x) * 0.5f;

            fx::RadialGradient(dl, ImVec2(cx, cmin.y + px(40)), cw * 0.95f, ch * 0.7f, White(0.035f), White(0.0f), 64);
            ui::Card(dl, cmin, cmax, px(18));
            const float sp = fmodf((float)now, 7.0f) / 1.8f;
            if (sp < 1.0f)
                fx::Shine(dl, cmin, cmax, sp, White(0.03f));

            // brand block
            DrawLogo(dl, ImVec2(cx, cmin.y + px(66)), px(40));
            const ImVec2 ts = ui::SpacedSize(F.bold, 26.0f, "VOID", px(6));
            ui::TextSpaced(dl, F.bold, 26.0f, ImVec2(cx - ts.x * 0.5f, cmin.y + px(104)), Gray(0.97f), "VOID", px(6));
            TextCentered(dl, F.regular, 13.5f, cx, cmin.y + px(144), Gray(0.50f), L(PremiumSystemOptimizer));

            // form
            const float fx0 = cmin.x + px(32), fw = cw - px(64);
            ui::TextSpaced(dl, F.medium, 10.5f, ImVec2(fx0, cmin.y + px(188)), Gray(0.42f), L(LicenseKey), px(1.6f));

            const float gbw = px(88);
            ImGui::SetCursorScreenPos(ImVec2(fx0 + fw - gbw, cmin.y + px(183)));
            ImGui::BeginDisabled(g_waiting);
            char genLabel[64]; snprintf(genLabel, sizeof(genLabel), "%s###gen", L(Generate));
            if (ui::Button(genLabel, ImVec2(gbw, px(24)), ButtonStyle::Ghost, Icon::Bolt))
                GenerateKey();
            ImGui::EndDisabled();

            ImGui::SetCursorScreenPos(ImVec2(fx0, cmin.y + px(208)));
            ImGui::BeginDisabled(g_waiting);
            const bool enter = ui::InputField("##license", "XXXX-XXXX-XXXX-XXXX", g_key, sizeof(g_key), Icon::Key, &g_reveal, fw,
                                              ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_EnterReturnsTrue);
            ImGui::SetCursorScreenPos(ImVec2(fx0, cmin.y + px(264)));
            ui::ToggleCard(L(RememberMe), nullptr, &g_remember, fw, false);
            ImGui::EndDisabled();

            ImGui::SetCursorScreenPos(ImVec2(fx0, cmin.y + px(320)));
            if (ui::Button(L(ActivateLicense), ImVec2(fw, px(46)), ButtonStyle::Primary, Icon::None, g_waiting) || enter)
                TryActivate();

            // status line
            const float sy = cmin.y + px(384);
            if (g_waiting)
            {
                TextCentered(dl, F.regular, 12.5f, cx, sy, Gray(0.55f), L(ContactingServer));
            }
            else if (!g_loginError.empty())
            {
                const ImVec2 es = ui::TextSize(F.medium, 12.5f, g_loginError.c_str());
                const float  x  = cx - (es.x + px(22)) * 0.5f;
                dl->AddCircleFilled(ImVec2(x + px(8), sy + es.y * 0.5f), px(8), White(0.92f), 20);
                icons::Draw(dl, Icon::Warning, ImVec2(x + px(8), sy + es.y * 0.5f), px(9), Gray(0.05f), px(1.6f));
                ui::Text(dl, F.medium, 12.5f, ImVec2(x + px(22), sy), Gray(0.88f), g_loginError.c_str());
            }
            else
            {
                char demoMsg[128];
                snprintf(demoMsg, sizeof(demoMsg), "%s \xC2\xB7 %s", L(DemoMode), L(AnyKeyAccepted));
                TextCentered(dl, F.regular, 12.5f, cx, sy, Gray(0.38f), demoMsg);
            }

            // footer
            const float fy = cmax.y - px(50);
            dl->AddLine(ImVec2(cmin.x + px(1), fy), ImVec2(cmax.x - px(1), fy), White(0.06f));
            ui::TextSpaced(dl, F.medium, 10.0f, ImVec2(fx0, fy + px(19)), Gray(0.38f), "HWID", px(1.4f));
            ui::Text(dl, F.regular, 12.0f, ImVec2(fx0 + px(44), fy + px(17)), Gray(0.62f), sys::Hwid().c_str());
            char ver[16];
            snprintf(ver, sizeof(ver), "v%s", kVersion);
            const ImVec2 vs = ui::TextSize(F.regular, 12.0f, ver);
            ui::Text(dl, F.regular, 12.0f, ImVec2(cmax.x - px(32) - vs.x, fy + px(17)), Gray(0.35f), ver);
        }

        // ------------------------------------------------------------------ loading

        void DrawLoading(const ImVec2& ds, float slide)
        {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            const auto& F = theme::fonts;
            const float t   = (float)(ImGui::GetTime() - g_screenStart);
            const float dur = 2.6f;
            const float p   = ImSaturate(t / dur);
            const ImVec2 c(ds.x * 0.5f, ds.y * 0.5f - px(50) + slide);

            dl->AddCircle(c, px(48), White(0.06f), 72, px(1.5f));
            ui::Spinner(dl, c, px(48), px(1.8f), White(0.85f));
            DrawLogo(dl, c, px(44));

            const std::string welcome = "Welcome back, " + license::Current().user;
            TextCentered(dl, F.bold, 20.0f, c.x, c.y + px(76), Gray(0.97f), welcome.c_str());

            static const char* steps[] = { "Establishing secure session", "Verifying license integrity", "Loading modules", "Preparing interface" };
            const int idx = ImMin((int)(p * 4.0f), 3);
            TextCentered(dl, F.regular, 13.0f, c.x, c.y + px(108), Gray(0.50f), steps[idx]);

            ImGui::SetCursorScreenPos(ImVec2(c.x - px(150), c.y + px(142)));
            ui::ProgressBar("##boot", p, ImVec2(px(300), px(4)));

            char pct[8];
            snprintf(pct, sizeof(pct), "%d%%", (int)(p * 100.0f));
            TextCentered(dl, F.medium, 11.5f, c.x, c.y + px(156), Gray(0.42f), pct);

            if (t > dur + 0.3f)
                GoTo(Screen::Main);
        }

        // ------------------------------------------------------------------ pages

        void StatCard(const ImVec2& mn, const ImVec2& sz, Icon icon, const char* label, const char* value, const char* sub, float frac)
        {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            const auto& F  = theme::fonts;
            const ImVec2 mx = mn + sz;
            const ImGuiID id = ImGui::GetID(label);
            const bool  hov = ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(mn, mx);
            const float hv  = ui::Anim(ui::Key(id, "h"), hov ? 1.0f : 0.0f, 12.0f);
            const float af  = ui::Anim(ui::Key(id, "f"), frac, 6.0f);

            ui::Card(dl, mn, mx, px(14), hv);
            const ImVec2 ib = mn + px(16, 16);
            dl->AddRectFilled(ib, ib + px(30, 30), White(0.06f + 0.04f * hv), px(8));
            dl->AddRect(ib, ib + px(30, 30), White(0.08f), px(8), 0, ImMax(1.0f, px(1)));
            icons::Draw(dl, icon, ib + px(15, 15), px(15), Gray(0.92f), px(1.4f));

            ui::Text(dl, F.medium, 13.0f, ImVec2(mn.x + px(56), mn.y + px(22)), Gray(0.60f), label);
            ui::Text(dl, F.bold, 24.0f, ImVec2(mn.x + px(16), mn.y + px(54)), Gray(0.97f), value);
            ui::Text(dl, F.regular, 12.0f, ImVec2(mn.x + px(16), mn.y + px(88)), Gray(0.42f), sub);

            const ImVec2 b0(mn.x + px(16), mx.y - px(15)), b1(mx.x - px(16), mx.y - px(12));
            dl->AddRectFilled(b0, b1, White(0.07f), px(2));
            dl->AddRectFilled(b0, ImVec2(b0.x + (b1.x - b0.x) * ImSaturate(af), b1.y), White(0.9f), px(2));
        }

        void PageDashboard(float cw)
        {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            const auto& F  = theme::fonts;
            const auto& s  = sys::Get();
            const double now = ImGui::GetTime();
            const float gap = px(14);
            char v[64], sub[64];

            // stat row
            ImVec2 p = ImGui::GetCursorScreenPos();
            const float sw = floorf((cw - gap * 3.0f) / 4.0f), sh = px(128);
            const float health = ui::Anim(ImGui::GetID("##health"), g_health, 3.0f);

            snprintf(v, sizeof(v), "%.0f%%", s.cpu * 100.0f);
            snprintf(sub, sizeof(sub), L(LogicalThreads), s.threads);
            StatCard(p, ImVec2(sw, sh), Icon::Cpu, L(Processor), v, sub, s.cpu);

            snprintf(v, sizeof(v), "%.1f GB", s.ramUsedGB);
            snprintf(sub, sizeof(sub), L(OfGBInUse), s.ramTotalGB);
            StatCard(p + ImVec2((sw + gap) * 1.0f, 0), ImVec2(sw, sh), Icon::Memory, L(Memory), v, sub, s.ramFrac);

            snprintf(v, sizeof(v), "%.0f GB", s.diskFreeGB);
            snprintf(sub, sizeof(sub), L(FreeOfGB), s.diskTotalGB);
            StatCard(p + ImVec2((sw + gap) * 2.0f, 0), ImVec2(sw, sh), Icon::Disk, L(Storage), v, sub, s.diskFrac);

            snprintf(v, sizeof(v), "%.0f", health);
            StatCard(p + ImVec2((sw + gap) * 3.0f, 0), ImVec2(sw, sh), Icon::Shield, L(Health), v,
                     health >= 92.0f ? L(Excellent) : health >= 75.0f ? L(Good) : L(NeedsAttention), health / 100.0f);
            ImGui::Dummy(ImVec2(cw, sh));

            // graph + optimize
            p = ImGui::GetCursorScreenPos();
            const float gw = floorf((cw - gap) * 0.64f), rh = px(236);
            ui::Card(dl, p, p + ImVec2(gw, rh));
            ui::Text(dl, F.bold, 15.0f, p + px(18, 16), Gray(0.96f), L(ProcessorLoad));
            ui::Text(dl, F.regular, 12.5f, p + px(18, 38), Gray(0.45f), L(LiveLast30s));

            const float cur = ui::Anim(ImGui::GetID("##cpucur"), g_cpu.back() * 100.0f, 6.0f);
            snprintf(v, sizeof(v), "%.0f%%", cur);
            const ImVec2 cs = ui::TextSize(F.bold, 18.0f, v);
            ui::Text(dl, F.bold, 18.0f, ImVec2(p.x + gw - px(18) - cs.x, p.y + px(18)), Gray(0.97f), v);

            float peak = 0.0f;
            for (float c : g_cpu) peak = ImMax(peak, c);
            const float vmax  = ui::Anim(ImGui::GetID("##cpumax"), ImMax(0.25f, peak * 1.3f), 3.0f);
            const float scroll = (float)((now - g_lastSample) / 0.5);
            ui::Graph(dl, p + px(18, 70), p + ImVec2(gw - px(18), rh - px(18)), g_cpu.data(), (int)g_cpu.size(), 0.0f, vmax, scroll);

            const ImVec2 o(p.x + gw + gap, p.y), osz(cw - gw - gap, rh);
            ui::Card(dl, o, o + osz);
            ui::Text(dl, F.bold, 15.0f, o + px(18, 16), Gray(0.96f), L(QuickOptimize));
            ui::Text(dl, F.regular, 12.5f, o + px(18, 38), Gray(0.45f), L(OneClickEvery));

            const float prog  = g_optimizing ? ImSaturate((float)(now - g_optStart) / 3.2f) : 0.0f;
            const float ringV = ui::Anim(ImGui::GetID("##ring"), g_optimizing ? prog : health / 100.0f, 6.0f);
            const ImVec2 rc(o.x + osz.x * 0.5f, o.y + px(114));
            ui::Ring(dl, rc, px(40), px(6), ringV);
            snprintf(v, sizeof(v), g_optimizing ? "%.0f%%" : "%.0f", g_optimizing ? prog * 100.0f : health);
            TextCentered(dl, F.bold, 22.0f, rc.x, rc.y - px(19), Gray(0.97f), v);
            TextCentered(dl, F.regular, 11.0f, rc.x, rc.y + px(8), Gray(0.45f), g_optimizing ? L(Optimizing) : L(HealthScore));

            ImGui::SetCursorScreenPos(ImVec2(o.x + px(18), o.y + rh - px(58)));
            if (ui::Button(L(OptimizeNow), ImVec2(osz.x - px(36), px(40)), ButtonStyle::Primary, Icon::Bolt, g_optimizing))
            {
                g_optimizing = true;
                g_optStart   = now;
            }
            ImGui::SetCursorScreenPos(p);
            ImGui::Dummy(ImVec2(cw, rh));

            // info cards
            const float hw = floorf((cw - gap) * 0.5f);
            const unsigned long long up = s.uptimeSec;
            char uptime[32];
            snprintf(uptime, sizeof(uptime), "%llu h %02llu m", up / 3600ULL, (up / 60ULL) % 60ULL);
            static const std::string os = sys::OsName(), pc = sys::ComputerName(), user = sys::UserName(),
                                     cpuN = sys::CpuName(), gpuN = sys::GpuName();

            ui::BeginCard("##system", hw, L(System));
            {
                const float w = ImGui::GetContentRegionAvail().x;
                InfoRow(L(OperatingSystem), os.c_str(), w);
                InfoRow(L(Processor), cpuN.c_str(), w);
                InfoRow(L(Graphics), gpuN.c_str(), w);
                InfoRow(L(Computer), pc.c_str(), w);
                InfoRow(L(User), user.c_str(), w);
                InfoRow(L(Uptime), uptime, w, false);
            }
            ui::EndCard();
            ImGui::SameLine(0, gap);
            ui::BeginCard("##subscription", hw, L(Subscription));
            {
                const float w = ImGui::GetContentRegionAvail().x;
                const auto& li = license::Current();
                InfoRow(L(Plan), li.plan.c_str(), w);
                InfoRow(L(Status), L(Active), w);
                InfoRow(L(Expires), li.expires.c_str(), w);
                InfoRow("HWID", li.hwid.c_str(), w, false);
            }
            ui::EndCard();
        }

        void PageCleaner(float cw)
        {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            const auto& F  = theme::fonts;
            const float gap = px(14);
            const int   n   = (int)g_cats.size();

            ImVec2 p = ImGui::GetCursorScreenPos();
            const float hh = px(128);
            ui::Card(dl, p, p + ImVec2(cw, hh));

            const float total = TotalFound();
            const int   cur   = ImMin((int)(g_cleanP * n), n - 1);
            std::string big;
            char cap[128];
            switch (g_cleanState)
            {
            case CleanState::Idle:
                big = L(ReadyToScan);
                snprintf(cap, sizeof(cap), "%s", L(SelectCategories));
                break;
            case CleanState::Scanning:
                big = FormatSize(total);
                snprintf(cap, sizeof(cap), "Scanning %s...", lang::Get(kCatKeys[cur].nameKey));
                break;
            case CleanState::Ready:
            {
                int k = 0;
                for (const Category& c : g_cats) k += (c.enabled && c.found > 0.5f) ? 1 : 0;
                big = FormatSize(total);
                snprintf(cap, sizeof(cap), "%s \xC2\xB7 %s", L(JunkFound), L(ReadyToClean));
                break;
            }
            case CleanState::Cleaning:
                big = FormatSize(total);
                snprintf(cap, sizeof(cap), "Cleaning %s...", lang::Get(kCatKeys[cur].nameKey));
                break;
            case CleanState::Done:
                big = L(AllClean);
                snprintf(cap, sizeof(cap), "%s %s \xC2\xB7 %s", FormatSize(g_freed).c_str(), L(Freed), L(Spotless));
                break;
            }
            ui::Text(dl, F.bold, 28.0f, p + px(22, 20), Gray(0.97f), big.c_str());
            ui::Text(dl, F.regular, 13.0f, p + px(22, 62), Gray(0.50f), cap);

            const bool  scanning = g_cleanState == CleanState::Scanning;
            const bool  cleaning = g_cleanState == CleanState::Cleaning;
            const float bw1 = px(112), bw2 = px(132), bh = px(40);

            ImGui::SetCursorScreenPos(ImVec2(p.x + cw - px(22) - bw2 - px(10) - bw1, p.y + px(24)));
            ImGui::BeginDisabled(cleaning);
            char scanLabel[64];
            snprintf(scanLabel, sizeof(scanLabel), "%s###scan", g_cleanState == CleanState::Idle ? L(Scan) : L(Rescan));
            if (ui::Button(scanLabel, ImVec2(bw1, bh), ButtonStyle::Secondary, Icon::Search, scanning))
                StartScan();
            ImGui::EndDisabled();

            ImGui::SetCursorScreenPos(ImVec2(p.x + cw - px(22) - bw2, p.y + px(24)));
            ImGui::BeginDisabled(!cleaning && !(g_cleanState == CleanState::Ready && total > 0.5f));
            if (ui::Button(L(CleanNow), ImVec2(bw2, bh), ButtonStyle::Primary, Icon::Cleaner, cleaning))
                StartClean();
            ImGui::EndDisabled();

            const float frac = (scanning || cleaning) ? g_cleanP
                             : (g_cleanState == CleanState::Ready || g_cleanState == CleanState::Done) ? 1.0f : 0.0f;
            ImGui::SetCursorScreenPos(ImVec2(p.x + px(22), p.y + hh - px(28)));
            ui::ProgressBar("##cleanprog", frac, ImVec2(cw - px(44), px(5)));
            ImGui::SetCursorScreenPos(p);
            ImGui::Dummy(ImVec2(cw, hh));

            ui::SectionLabel(L(Categories));

            const float colw = floorf((cw - gap) * 0.5f);
            ImGui::BeginDisabled(scanning || cleaning);
            ImVec2 row;
            for (int i = 0; i < n; ++i)
            {
                Category& c = g_cats[i];
                const int col = i % 2;
                if (col == 0)
                    row = ImGui::GetCursorScreenPos();
                ImGui::SetCursorScreenPos(row + ImVec2(col * (colw + gap), 0));

                std::string right;
                if (!c.enabled)                          right = "Skipped";
                else if (g_cleanState == CleanState::Idle) right = "";
                else if (scanning && c.found <= 0.0f)    right = "...";
                else                                     right = FormatSize(c.found);

                ui::CheckRow(lang::Get(kCatKeys[i].nameKey), lang::Get(kCatKeys[i].descKey), right.c_str(), &c.enabled, colw);
                if (col == 1 || i == n - 1)
                {
                    ImGui::SetCursorScreenPos(row);
                    ImGui::Dummy(ImVec2(cw, px(64)));
                }
            }
            ImGui::EndDisabled();
        }

        void ToggleGrid(std::vector<Tweak>& list, float cw)
        {
            const float gap  = px(14);
            const float colw = floorf((cw - gap) * 0.5f);
            ImVec2 row;
            for (int i = 0; i < (int)list.size(); ++i)
            {
                const int col = i % 2;
                if (col == 0)
                    row = ImGui::GetCursorScreenPos();
                ImGui::SetCursorScreenPos(row + ImVec2(col * (colw + gap), 0));
                ui::ToggleCard(list[i].name, list[i].desc, &list[i].on, colw);
                if (col == 1 || i == (int)list.size() - 1)
                {
                    ImGui::SetCursorScreenPos(row);
                    ImGui::Dummy(ImVec2(cw, px(64)));
                }
            }
        }

        int g_ramPick = -1;   // chosen preset, -1 until Init() reads the registry

        // Grid of RAM-size chips + an apply bar. Highlights the preset matching installed RAM.
        void RamOptimizer(float cw)
        {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            const auto& F  = theme::fonts;
            const int   n  = ram::PresetCount();
            const int   detected = ram::DetectedGB();

            const ImVec2 p0 = ImGui::GetCursorScreenPos();
            const float  ch = px(238);
            ui::Card(dl, p0, p0 + ImVec2(cw, ch));
            ui::Text(dl, F.bold, 15.0f, p0 + px(18, 16), Gray(0.96f), L(RamOptimization));
            ui::Text(dl, F.regular, 12.5f, p0 + px(18, 38), Gray(0.45f), L(RamOptDesc));

            // installed-memory badge, top right
            {
                char badge[64];
                snprintf(badge, sizeof(badge), "%s \xC2\xB7 %d GB", L(InstalledRam), detected);
                const ImVec2 bs = ui::TextSize(F.medium, 12.0f, badge);
                const ImVec2 b0(p0.x + cw - px(18) - bs.x - px(20), p0.y + px(18));
                const ImVec2 b1 = b0 + ImVec2(bs.x + px(20), px(24));
                dl->AddRectFilled(b0, b1, White(0.06f), px(7));
                dl->AddRect(b0, b1, White(0.12f), px(7), 0, ImMax(1.0f, px(1)));
                ui::Text(dl, F.medium, 12.0f, ImVec2(b0.x + px(10), b0.y + (px(24) - bs.y) * 0.5f), Gray(0.80f), badge);
            }

            // chip grid
            const float gap = px(8), cols = 7.0f;
            const float chipW = floorf((cw - px(36) - gap * (cols - 1)) / cols), chipH = px(38);
            for (int i = 0; i < n; ++i)
            {
                const int   col = i % (int)cols, rowi = i / (int)cols;
                const ImVec2 c0(p0.x + px(18) + col * (chipW + gap), p0.y + px(74) + rowi * (chipH + gap));
                const ImVec2 c1 = c0 + ImVec2(chipW, chipH);

                ImGui::SetCursorScreenPos(c0);
                char id[32];
                snprintf(id, sizeof(id), "##ram%d", i);
                const bool clicked = ImGui::InvisibleButton(id, ImVec2(chipW, chipH));
                const bool hov     = ImGui::IsItemHovered();
                if (clicked) g_ramPick = i;

                const bool sel  = g_ramPick == i;
                const bool rec  = ram::PresetGB(i) == detected;
                const ImGuiID aid = ImGui::GetID(id);
                const float   sa  = ui::Anim(ui::Key(aid, "s"), sel ? 1.0f : 0.0f, 14.0f);
                const float   ha  = ui::Anim(ui::Key(aid, "h"), hov ? 1.0f : 0.0f, 14.0f);

                dl->AddRectFilled(c0, c1, White(0.045f + 0.05f * ha + 0.80f * sa), px(9));
                dl->AddRect(c0, c1, White(sel ? 0.0f : (rec ? 0.26f : 0.09f)), px(9), 0, ImMax(1.0f, px(1)));

                const char*  nm = ram::PresetName(i);
                const ImVec2 ts = ui::TextSize(sel ? F.bold : F.medium, 12.5f, nm);
                ui::Text(dl, sel ? F.bold : F.medium, 12.5f,
                         ImVec2((c0.x + c1.x - ts.x) * 0.5f, (c0.y + c1.y - ts.y) * 0.5f),
                         sel ? Gray(0.05f) : Gray(rec ? 0.92f : 0.60f), nm);

                // small dot marks the preset matching installed RAM
                if (rec && !sel)
                    dl->AddCircleFilled(ImVec2(c1.x - px(8), c0.y + px(8)), px(2.5f), White(0.75f), 10);
            }

            // apply bar
            const float by = p0.y + ch - px(50);
            dl->AddLine(ImVec2(p0.x + px(1), by), ImVec2(p0.x + cw - px(1), by), White(0.06f));

            char cur[96];
            const int live = ram::CurrentPreset();
            snprintf(cur, sizeof(cur), "%s: %s", L(CurrentProfile),
                     live < 0 ? L(Custom) : ram::PresetName(live));
            ui::Text(dl, F.regular, 12.0f, ImVec2(p0.x + px(18), by + px(19)), Gray(0.50f), cur);

            const float bw = px(150);
            ImGui::SetCursorScreenPos(ImVec2(p0.x + cw - px(18) - bw, by + px(10)));
            ImGui::BeginDisabled(g_ramPick < 0 || g_ramPick == live);
            if (ui::Button(L(ApplyRamProfile), ImVec2(bw, px(32)), ButtonStyle::Primary, Icon::Memory))
            {
                if (ram::Apply(g_ramPick))
                    ui::Notify(Toast::Success, L(RamProfileApplied), L(RamRestartNote));
                else
                    ui::Notify(Toast::Warning, L(RamProfileApplied), L(NeedAdmin));
            }
            ImGui::EndDisabled();

            ImGui::SetCursorScreenPos(p0);
            ImGui::Dummy(ImVec2(cw, ch));
        }

        void PageTweaks(float cw)
        {
            const char* const cats[] = { L(Performance), L(Gaming), L(Privacy), L(Visual), L(Games), L(FiveM), L(Delay) };
            const double now = ImGui::GetTime();

            const ImVec2 p = ImGui::GetCursorScreenPos();
            const float segW = ImMin(px(640), cw - px(240));
            if (ui::Segmented("##tweakcat", cats, kTweakCats, &g_tweakCat, segW))
                g_tweakCatTime = now;

            std::vector<Tweak>& list = g_tweaks[g_tweakCat];
            const float bh = px(38), bwA = px(112), bwR = px(96);
            ImGui::SetCursorScreenPos(ImVec2(p.x + cw - bwA, p.y));
            if (ui::Button(L(Apply), ImVec2(bwA, bh), ButtonStyle::Primary, Icon::Check, g_applying))
            {
                g_applying   = true;
                g_applyStart = now;
            }
            ImGui::SetCursorScreenPos(ImVec2(p.x + cw - bwA - px(10) - bwR, p.y));
            if (ui::Button(L(Reset), ImVec2(bwR, bh), ButtonStyle::Secondary, Icon::Refresh))
            {
                for (Tweak& t : list)
                    t.on = false;
                ui::Notify(Toast::Info, "Defaults restored", "All tweaks in this category disabled");
            }
            ImGui::SetCursorScreenPos(p);
            ImGui::Dummy(ImVec2(cw, bh));

            int on = 0;
            for (const Tweak& t : list)
                on += t.on ? 1 : 0;
            char info[96];
            snprintf(info, sizeof(info), "%d of %d %s tweaks enabled", on, (int)list.size(), cats[g_tweakCat]);
            ui::Label(theme::fonts.regular, 12.5f, 0.45f, info);

            const float a = ImSaturate((float)(now - g_tweakCatTime) / 0.25f);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * a);
            ToggleGrid(list, cw);
            ImGui::PopStyleVar();

            ui::SectionLabel(L(RamOptimization));
            RamOptimizer(cw);
        }

        void PageNetwork(float cw)
        {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            const auto& F  = theme::fonts;
            const double now = ImGui::GetTime();
            char v[64];

            // latency graph
            ImVec2 p = ImGui::GetCursorScreenPos();
            const float gh = px(210);
            ui::Card(dl, p, p + ImVec2(cw, gh));
            ui::Text(dl, F.bold, 15.0f, p + px(18, 16), Gray(0.96f), L(Latency));
            {
                char rtLabel[128];
                snprintf(rtLabel, sizeof(rtLabel), "%s \xC2\xB7 %s", L(RoundTrip), L(Simulated));
                ui::Text(dl, F.regular, 12.5f, p + px(18, 38), Gray(0.45f), rtLabel);
            }

            const float cur = ui::Anim(ImGui::GetID("##pingcur"), g_ping.back(), 6.0f);
            float mean = 0.0f, var = 0.0f;
            for (int i = (int)g_ping.size() - 10; i < (int)g_ping.size(); ++i) mean += g_ping[i] / 10.0f;
            for (int i = (int)g_ping.size() - 10; i < (int)g_ping.size(); ++i) var += (g_ping[i] - mean) * (g_ping[i] - mean) / 10.0f;
            snprintf(v, sizeof(v), "%.0f ms", cur);
            const ImVec2 cs = ui::TextSize(F.bold, 18.0f, v);
            ui::Text(dl, F.bold, 18.0f, ImVec2(p.x + cw - px(18) - cs.x, p.y + px(14)), Gray(0.97f), v);
            snprintf(v, sizeof(v), "%s %.1f ms", L(Jitter), sqrtf(var));
            const ImVec2 js = ui::TextSize(F.regular, 12.0f, v);
            ui::Text(dl, F.regular, 12.0f, ImVec2(p.x + cw - px(18) - js.x, p.y + px(40)), Gray(0.45f), v);

            ui::Graph(dl, p + px(18, 70), p + ImVec2(cw - px(18), gh - px(18)), g_ping.data(), (int)g_ping.size(), 0.0f, 60.0f,
                      (float)((now - g_lastSample) / 0.5));
            ImGui::Dummy(ImVec2(cw, gh));

            // DNS
            static const char* const dns[] = { "Automatic", "Cloudflare", "Google", "Quad9" };
            p = ImGui::GetCursorScreenPos();
            const float dh = px(136);
            ui::Card(dl, p, p + ImVec2(cw, dh));
            ui::Text(dl, F.bold, 15.0f, p + px(18, 16), Gray(0.96f), L(DnsProvider));
            ui::Text(dl, F.regular, 12.5f, p + px(18, 38), Gray(0.45f), L(ResolverUsed));

            const float fbw = px(160);
            ImGui::SetCursorScreenPos(p + px(18, 78));
            if (ui::Segmented("##dns", dns, 4, &g_dns, ImMin(px(440), cw - px(36) - fbw - px(14))))
            {
                network::SetDns(g_dns);
                const std::string msg = std::string("Now resolving through ") + dns[g_dns];
                ui::Notify(Toast::Success, "DNS updated", msg.c_str());
            }
            ImGui::SetCursorScreenPos(ImVec2(p.x + cw - px(18) - fbw, p.y + px(78)));
            if (ui::Button(L(FlushDnsCache), ImVec2(fbw, px(38)), ButtonStyle::Secondary, Icon::Refresh))
            {
                network::FlushDns();
                ui::Notify(Toast::Success, "DNS cache flushed", "Resolver cache cleared");
            }
            ImGui::SetCursorScreenPos(p);
            ImGui::Dummy(ImVec2(cw, dh));

            // connection info
            {
                static std::string adp = network::AdapterName(), lip = network::LocalIP(), gip = network::GatewayIP();
                p = ImGui::GetCursorScreenPos();
                const float ih = px(136);
                ui::Card(dl, p, p + ImVec2(cw, ih));
                ui::Text(dl, F.bold, 15.0f, p + px(18, 16), Gray(0.96f), L(Connection));
                ui::Text(dl, F.regular, 12.5f, p + px(18, 38), Gray(0.45f), L(ActiveAdapter));
                const float rw = cw - px(36);
                ImGui::SetCursorScreenPos(p + px(18, 64));
                ImGui::BeginGroup();
                InfoRow("Adapter", adp.c_str(), rw);
                InfoRow("Local IP", lip.c_str(), rw);
                InfoRow("Gateway", gip.c_str(), rw, false);
                ImGui::EndGroup();
                ImGui::SetCursorScreenPos(p);
                ImGui::Dummy(ImVec2(cw, ih));
            }

            ui::SectionLabel(L(Optimizations));
            {
                const float gap = px(14);
                const float colw = floorf((cw - gap) * 0.5f);
                ImVec2 row;
                for (int i = 0; i < (int)g_netTweaks.size(); ++i)
                {
                    const int col = i % 2;
                    if (col == 0) row = ImGui::GetCursorScreenPos();
                    ImGui::SetCursorScreenPos(row + ImVec2(col * (colw + gap), 0));
                    bool prev = g_netTweaks[i].on;
                    ui::ToggleCard(g_netTweaks[i].name, g_netTweaks[i].desc, &g_netTweaks[i].on, colw);
                    if (g_netTweaks[i].on != prev) network::ApplyTweak(i, g_netTweaks[i].on);
                    if (col == 1 || i == (int)g_netTweaks.size() - 1)
                    {
                        ImGui::SetCursorScreenPos(row);
                        ImGui::Dummy(ImVec2(cw, px(64)));
                    }
                }
            }
        }

        void SysInfoSection(const char* title, Icon icon, float cw)
        {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            const auto& F  = theme::fonts;
            const ImVec2 p  = ImGui::GetCursorScreenPos();
            const float  ih = px(24);
            dl->AddRectFilled(p, ImVec2(p.x + px(4), p.y + ih), White(0.80f), px(2));
            icons::Draw(dl, icon, ImVec2(p.x + px(20), p.y + ih * 0.5f), px(10), Gray(0.60f), px(1.2f));
            ui::TextSpaced(dl, F.bold, 11.5f, ImVec2(p.x + px(36), p.y + (ih - ui::TextSize(F.bold, 11.5f, title).y) * 0.5f),
                           Gray(0.55f), title, px(1.5f));
            ImGui::Dummy(ImVec2(cw, ih + px(6)));
        }

        void PageSystemInfo(float cw)
        {
            const auto& F  = theme::fonts;
            const float gap = px(14);
            const float hw  = floorf((cw - gap) * 0.5f);

            static bool gathered = false;
            if (!gathered) { sysdetail::Gather(); gathered = true; }
            const auto& d  = sysdetail::Get();
            static const std::string cpuN = sys::CpuName(), gpuN = sys::GpuName(),
                                     os = sys::OsName(), pc = sys::ComputerName();

            // Hardware section
            ImGui::BeginGroup();
            SysInfoSection(L(HardwareInfo), Icon::Chip, cw);

            ui::BeginCard("##sicpu", hw, L(Processor));
            {
                const float w = ImGui::GetContentRegionAvail().x;
                InfoRow(L(CPUName), cpuN.c_str(), w);
                char cores[16], threads[16];
                snprintf(cores, sizeof(cores), "%d", d.cpuCores);
                snprintf(threads, sizeof(threads), "%d", d.cpuThreads);
                InfoRow(L(CPUCores), cores, w);
                InfoRow(L(CPUThreads), threads, w);
                InfoRow(L(CPUClock), d.cpuClock.c_str(), w, false);
            }
            ui::EndCard();
            ImGui::SameLine(0, gap);
            ui::BeginCard("##sigpu", hw, L(GPUName));
            {
                const float w = ImGui::GetContentRegionAvail().x;
                InfoRow(L(GPUName), gpuN.c_str(), w);
                InfoRow(L(GPUVRAM), d.gpuVram.c_str(), w);
                InfoRow(L(GPUDriver), d.gpuDriver.c_str(), w);
                InfoRow(L(DirectXVersion), d.directX.c_str(), w, false);
            }
            ui::EndCard();

            ui::BeginCard("##siram", hw, L(RAMTotal));
            {
                const float w = ImGui::GetContentRegionAvail().x;
                InfoRow(L(RAMTotal), d.ramTotal.c_str(), w);
                InfoRow(L(RAMSpeed), d.ramSpeed.c_str(), w);
                InfoRow(L(RAMSlots), d.ramSlots.c_str(), w, false);
            }
            ui::EndCard();
            ImGui::SameLine(0, gap);
            ui::BeginCard("##siboard", hw, L(Motherboard));
            {
                const float w = ImGui::GetContentRegionAvail().x;
                InfoRow(L(Motherboard), d.motherboard.c_str(), w);
                InfoRow(L(BIOSVersion), d.biosVersion.c_str(), w);
                InfoRow(L(BIOSMode), d.biosMode.c_str(), w, false);
            }
            ui::EndCard();
            ImGui::EndGroup();

            // Security section
            ImGui::BeginGroup();
            SysInfoSection(L(SecurityInfo), Icon::Lock, cw);

            ui::BeginCard("##sisec", hw, L(SecurityInfo));
            {
                const float w = ImGui::GetContentRegionAvail().x;
                InfoRow(L(SecureBoot), d.secureBoot.c_str(), w);
                InfoRow(L(Virtualization), d.virtualization.c_str(), w);
                InfoRow(L(BIOSMode), d.biosMode.c_str(), w, false);
            }
            ui::EndCard();
            ImGui::SameLine(0, gap);
            ui::BeginCard("##sisw", hw, L(SoftwareInfo));
            {
                const float w = ImGui::GetContentRegionAvail().x;
                InfoRow(L(OperatingSystem), os.c_str(), w);
                InfoRow(L(InstallDate), d.installDate.c_str(), w);
                InfoRow(L(DisplayResolution), d.displayRes.c_str(), w);
                InfoRow(L(SystemLocale), d.systemLocale.c_str(), w);
                InfoRow(L(Computer), pc.c_str(), w, false);
            }
            ui::EndCard();
            ImGui::EndGroup();
        }

        void PageSettings(float cw)
        {
            const float gap  = px(14);
            const float colw = floorf((cw - gap) * 0.5f);

            ImGui::BeginGroup();
            ui::BeginCard("##visual", colw, L(VisualEffects), L(TuneBg));
            {
                const float w = ImGui::GetContentRegionAvail().x;
                auto& s = fx::settings;
                ui::ToggleCard(L(Particles), nullptr, &s.particles, w, false);
                ui::ToggleCard(L(ConnectionLines), nullptr, &s.lines, w, false);
                ui::ToggleCard(L(MouseInteraction), nullptr, &s.mouse, w, false);
                ui::ToggleCard(L(TopLight), nullptr, &s.glow, w, false);
                ui::ToggleCard(L(LightSweep), nullptr, &s.sweep, w, false);
                ImGui::Dummy(ImVec2(0, px(2)));
                ui::SliderInt(L(ParticleCount), &s.count, 20, 220, w);
                ui::Slider(L(ParticleSpeed), &s.speed, 0.2f, 3.0f, "%.1fx", w);
            }
            ui::EndCard();
            ImGui::EndGroup();

            ImGui::SameLine(0, gap);

            ImGui::BeginGroup();
            ui::BeginCard("##general", colw, L(General), L(AppBehaviour));
            {
                const float w = ImGui::GetContentRegionAvail().x;
                ui::ToggleCard(L(RememberLicense), nullptr, &g_remember, w, false);
                ui::ToggleCard(L(Notifications), nullptr, &ui::notificationsEnabled, w, false);
                ui::ToggleCard(L(LaunchStartup), nullptr, &g_startup, w, false);
                ui::ToggleCard(L(MinimizeToTray), nullptr, &g_tray, w, false);
            }
            ui::EndCard();

            // Language card
            ui::BeginCard("##langcard", colw, L(Language));
            {
                const float w = ImGui::GetContentRegionAvail().x;
                static const char* const langs[] = { "English", "T\xC3\xBCrk\xC3\xA7\x65" };
                static int langIdx = (int)lang::Current();
                if (ui::Segmented("##lang", langs, 2, &langIdx, w))
                    lang::Set((lang::Id)langIdx);
            }
            ui::EndCard();

            ui::BeginCard("##account", colw, L(Account));
            {
                const float w = ImGui::GetContentRegionAvail().x;
                const auto& li = license::Current();
                const std::string masked = license::Mask(li.key);
                InfoRow(L(LicenseKey), masked.c_str(), w);
                InfoRow(L(Plan), li.plan.c_str(), w);
                InfoRow(L(Expires), li.expires.c_str(), w, false);
                ImGui::Dummy(ImVec2(0, px(4)));
                if (ui::Button(L(SignOut), ImVec2(w, px(40)), ButtonStyle::Secondary, Icon::Logout))
                    SignOut();
                char ver[64];
                snprintf(ver, sizeof(ver), "VOID v%s \xC2\xB7 build %s", kVersion, __DATE__);
                ui::Label(theme::fonts.regular, 11.5f, 0.34f, ver);
            }
            ui::EndCard();
            ImGui::EndGroup();
        }

        // ------------------------------------------------------------------ main shell

        void DrawSidebar(const ImVec2& ds, float sbw)
        {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            const auto& F  = theme::fonts;
            const double now = ImGui::GetTime();

            dl->AddRectFilled(ImVec2(0, 0), ImVec2(sbw, ds.y), ImGui::GetColorU32(ImVec4(0.03f, 0.03f, 0.035f, 0.80f)));
            dl->AddRectFilledMultiColor(ImVec2(sbw - ImMax(1.0f, px(1)), 0), ImVec2(sbw, ds.y), White(0.10f), White(0.10f), White(0.02f), White(0.02f));

            // brand
            DrawLogo(dl, ImVec2(px(34), px(39)), px(24), 0.8f);
            ui::TextSpaced(dl, F.bold, 16.0f, ImVec2(px(58), px(27)), Gray(0.97f), "VOID", px(3));
            const float vw = ui::SpacedSize(F.bold, 16.0f, "VOID", px(3)).x;
            const ImVec2 bs = ui::SpacedSize(F.medium, 8.5f, "PREMIUM", px(1.2f));
            const ImVec2 b0(px(58) + vw + px(8), px(31));
            const ImVec2 b1 = b0 + ImVec2(bs.x + px(12), px(16));
            dl->AddRectFilled(b0, b1, White(0.06f), px(5));
            dl->AddRect(b0, b1, White(0.22f), px(5), 0, ImMax(1.0f, px(1)));
            ui::TextSpaced(dl, F.medium, 8.5f, ImVec2(b0.x + px(6), b0.y + (px(16) - bs.y) * 0.5f), Gray(0.85f), "PREMIUM", px(1.2f));
            const float bsh = fmodf((float)now, 4.0f) / 1.2f;
            if (bsh < 1.0f)
                fx::Shine(dl, b0, b1, bsh, White(0.35f));

            ImGui::SetCursorScreenPos(ImVec2(px(20), px(86)));
            ui::SectionLabel("MENU");

            // tabs + sliding indicator
            const float tabH = px(40), gap = px(4), tx = px(12), tw = sbw - px(24), ty0 = px(108);
            const float iy = ui::Anim(ImGui::GetID("##tabind"), ty0 + g_tab * (tabH + gap), 16.0f);
            dl->AddRectFilled(ImVec2(tx, iy), ImVec2(tx + tw, iy + tabH), White(0.065f), px(9));
            dl->AddRect(ImVec2(tx, iy), ImVec2(tx + tw, iy + tabH), White(0.06f), px(9), 0, ImMax(1.0f, px(1)));
            fx::RadialGradient(dl, ImVec2(0, iy + tabH * 0.5f), px(30), px(26), White(0.22f), White(0.0f), 24);
            dl->AddRectFilled(ImVec2(0, iy + px(11)), ImVec2(px(3), iy + tabH - px(11)), White(0.95f), px(2));

            for (int i = 0; i < kTabCount; ++i)
            {
                ImGui::SetCursorScreenPos(ImVec2(tx, ty0 + i * (tabH + gap)));
                if (ui::Tab(lang::Get(kTabNameKeys[i]), kTabIcons[i], g_tab == i, ImVec2(tw, tabH)) && g_tab != i)
                {
                    g_tab     = i;
                    g_tabTime = now;
                }
            }

            // user card
            const ImVec2 u0(tx, ds.y - px(76)), u1(tx + tw, ds.y - px(14));
            const float  cy = (u0.y + u1.y) * 0.5f;
            ui::Card(dl, u0, u1, px(12));
            const ImVec2 av(u0.x + px(28), cy);
            fx::RadialGradient(dl, av, px(26), px(26), White(0.12f), White(0.0f), 24);
            dl->AddCircleFilled(av, px(16), White(0.94f), 32);

            const std::string& user = license::Current().user;
            const char initial[2] = { user.empty() ? 'U' : (char)toupper((unsigned char)user[0]), 0 };
            const ImVec2 is = ui::TextSize(F.bold, 15.0f, initial);
            ui::Text(dl, F.bold, 15.0f, av - is * 0.5f, Gray(0.05f), initial);

            dl->PushClipRect(u0, ImVec2(u1.x - px(42), u1.y), true);
            ui::Text(dl, F.medium, 13.5f, ImVec2(u0.x + px(52), cy - px(18)), Gray(0.95f), user.c_str());
            dl->AddCircleFilled(ImVec2(u0.x + px(55), cy + px(9)), px(3), White(0.9f), 12);
            ui::Text(dl, F.regular, 12.0f, ImVec2(u0.x + px(63), cy + px(1)), Gray(0.50f), license::Current().plan.c_str());
            dl->PopClipRect();

            ImGui::SetCursorScreenPos(ImVec2(u1.x - px(38), cy - px(14)));
            if (ui::IconButton("##signout", Icon::Logout, px(28, 28), px(14)))
                SignOut();
        }

        void DrawMain(const ImVec2& ds, float slide)
        {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            const auto& F  = theme::fonts;
            const double now = ImGui::GetTime();
            const float sbw = px(212);

            DrawSidebar(ds, sbw);

            const float hx = sbw + px(28);
            ui::Text(dl, F.bold, 22.0f, ImVec2(hx, px(18)), Gray(0.97f), lang::Get(kTabNameKeys[g_tab]));
            ui::Text(dl, F.regular, 13.0f, ImVec2(hx, px(50)), Gray(0.48f), lang::Get(kTabSubKeys[g_tab]));
            dl->AddRectFilledMultiColor(ImVec2(sbw, px(80)), ImVec2(ds.x, px(80) + ImMax(1.0f, px(1))), White(0.07f), White(0.0f), White(0.0f), White(0.07f));

            float pa = ImSaturate((float)(now - g_tabTime) / 0.3f);
            pa = 1.0f - (1.0f - pa) * (1.0f - pa) * (1.0f - pa);
            const ImVec2 cmin(hx, px(96) + (1.0f - pa) * px(12) + slide);
            const ImVec2 csize(ds.x - px(14) - hx, ds.y - px(14) - cmin.y);

            ImGui::SetCursorScreenPos(cmin);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * pa);
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(px(14), px(14)));
            char id[16];
            snprintf(id, sizeof(id), "##page%d", g_tab);
            ImGui::BeginChild(id, csize, ImGuiChildFlags_None, ImGuiWindowFlags_NoBackground);
            {
                const float cw = ImGui::GetContentRegionAvail().x - px(14);
                switch (g_tab)
                {
                case 0: PageDashboard(cw);  break;
                case 1: PageCleaner(cw);    break;
                case 2: PageTweaks(cw);     break;
                case 3: PageNetwork(cw);    break;
                case 4: PageSystemInfo(cw); break;
                case 5: PageSettings(cw);   break;
                }
                ImGui::Dummy(ImVec2(0, px(4)));
            }
            ImGui::EndChild();
            ImGui::PopStyleVar(2);
        }
    }

    // ====================================================================== public

    void Init(HWND hwnd, float corner_radius)
    {
        g_hwnd   = hwnd;
        g_corner = corner_radius;
        sys::Update();

        for (int c = 0; c < kTweakCats; ++c)
            for (int i = 0; i < (int)g_tweaks[c].size(); ++i)
                g_tweaks[c][i].on = tweaks::Read(c, i);
        for (int i = 0; i < (int)g_netTweaks.size(); ++i)
            g_netTweaks[i].on = network::ReadTweak(i);

        g_ramPick = ram::CurrentPreset();

        const std::string saved = license::LoadSaved();
        if (!saved.empty())
        {
            strncpy_s(g_key, saved.c_str(), _TRUNCATE);
            g_remember = true;
        }
        for (float& v : g_ping)
            v = 17.0f + Rand(-3.0f, 3.0f);

        ui::AnimSet(ImHashStr("##winalpha"), 0.0f); // fade in on launch

#ifdef VOID_DEV
        // Dev build only: auto sign-in with a demo key (tabs are cycled in Frame()).
        strncpy_s(g_key, "LIFE-2026-ABCD-WXYZ", _TRUNCATE);
        g_remember = false;
        TryActivate();
#endif
    }

    bool WantsQuit() { return g_quit; }

    void Frame()
    {
        ImGuiIO& io = ImGui::GetIO();
        const ImVec2 ds = io.DisplaySize;
        if (ds.x <= 0.0f || ds.y <= 0.0f)
            return;

        const double now = ImGui::GetTime();
        UpdateData(now);

#ifdef VOID_DEV
        if (g_screen == Screen::Main && !g_switching)
        {
            const int devTab = (int)((now - g_screenStart) / 2.5) % kTabCount;
            if (devTab != g_tab)
            {
                g_tab     = devTab;
                g_tabTime = now;
                if (g_tab == 1 && g_cleanState == CleanState::Idle)
                    StartScan();
            }
        }
#endif

        const float winA = ui::Anim(ImHashStr("##winalpha"), g_closing ? 0.0f : 1.0f, g_closing ? 12.0f : 4.0f);
        if (g_closing && winA < 0.02f)
            g_quit = true;

        const float scrA = ui::Anim(ImHashStr("##screen"), g_switching ? 0.0f : 1.0f, g_switching ? 14.0f : 8.0f);
        if (g_switching && scrA < 0.03f)
        {
            g_screen      = g_next;
            g_switching   = false;
            g_screenStart = now;
            if (g_screen == Screen::Main)
            {
                g_tab     = 0;
                g_tabTime = now;
            }
        }

        fx::DrawBackground(ImGui::GetBackgroundDrawList(), ImVec2(0, 0), ds, winA);

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ds);
        ImGui::Begin("##root", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus |
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        const float slide = (1.0f - scrA) * px(14);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, winA * scrA);
        switch (g_screen)
        {
        case Screen::Login:   DrawLogin(ds, slide);   break;
        case Screen::Loading: DrawLoading(ds, slide); break;
        case Screen::Main:    DrawMain(ds, slide);    break;
        }
        ImGui::PopStyleVar();

        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, winA);
        DrawWindowControls(ds);
        ImGui::PopStyleVar();

        HandleDrag();
        ImGui::End();

        // light sweep also passes over the UI, notifications on top, then the window outline
        fx::DrawSweep(ImGui::GetForegroundDrawList(), ImVec2(0, 0), ds, winA * 0.5f);
        ui::RenderNotifications(ds);
        ImGui::GetForegroundDrawList()->AddRect(ImVec2(0.5f, 0.5f), ds - ImVec2(0.5f, 0.5f),
                                                IM_COL32(255, 255, 255, (int)(26.0f * winA)), g_corner, 0, 1.0f);
    }
}
