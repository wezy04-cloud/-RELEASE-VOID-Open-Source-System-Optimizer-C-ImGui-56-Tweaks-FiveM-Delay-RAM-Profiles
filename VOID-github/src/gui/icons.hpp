#pragma once
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui.h"

// Vector icons drawn with ImDrawList (no icon font needed).
enum class Icon
{
    None,
    Dashboard, Cleaner, Tweaks, Network, Settings,
    Key, User, Logout, Close, Minimize, Check,
    Eye, EyeOff, Cpu, Memory, Disk, Shield, Bolt,
    Info, Warning, Error, Search, Refresh,
    Monitor, Globe, Lock, Chip,
};

namespace icons
{
    // c = center, s = box size, th = stroke thickness (0 = auto)
    void Draw(ImDrawList* dl, Icon icon, const ImVec2& c, float s, ImU32 col, float th = 0.0f);
}
