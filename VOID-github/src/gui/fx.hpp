#pragma once
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui.h"

// Background effects: falling glow particles, constellation lines,
// top-down light (glow + rays) and the periodic top-to-bottom light sweep.
namespace fx
{
    struct Settings
    {
        bool  particles = true;
        bool  lines     = true;
        bool  mouse     = true;
        bool  glow      = true;
        bool  sweep     = true;
        int   count     = 90;
        float speed     = 1.0f;
    };
    extern Settings settings;

    // Primitives
    void RadialGradient(ImDrawList* dl, const ImVec2& center, float rx, float ry, ImU32 inner, ImU32 outer, int segments = 48);
    void GradientQuad(ImDrawList* dl, const ImVec2& a, const ImVec2& b, const ImVec2& c, const ImVec2& d,
                      ImU32 ca, ImU32 cb, ImU32 cc, ImU32 cd);
    // Diagonal glint across a rect. t in [0,1], col carries the peak alpha.
    void Shine(ImDrawList* dl, const ImVec2& mn, const ImVec2& mx, float t, ImU32 col);

    // Scene layers. alpha = global fade (window open/close).
    void DrawBackground(ImDrawList* dl, const ImVec2& mn, const ImVec2& mx, float alpha);
    void DrawSweep(ImDrawList* dl, const ImVec2& mn, const ImVec2& mx, float alpha);
}
