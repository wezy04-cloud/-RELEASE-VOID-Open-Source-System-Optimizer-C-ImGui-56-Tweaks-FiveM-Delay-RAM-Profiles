#pragma once
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui.h"

// Monochrome (black / white) design tokens + font set.
namespace theme
{
    extern float scale; // monitor DPI scale

    inline float  px(float v)          { return v * scale; }
    inline ImVec2 px(float x, float y) { return ImVec2(x * scale, y * scale); }

    struct Fonts
    {
        ImFont* regular = nullptr;
        ImFont* medium  = nullptr;
        ImFont* bold    = nullptr;
    };
    extern Fonts fonts;

    void Init(float dpi_scale); // load fonts + apply ImGui style

    // All of these respect ImGui's current style alpha (fades work everywhere).
    ImU32 White(float a = 1.0f);
    ImU32 Black(float a = 1.0f);
    ImU32 Gray(float v, float a = 1.0f); // v = brightness 0..1
}
