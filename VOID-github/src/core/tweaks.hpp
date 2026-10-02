#pragma once

namespace tweaks
{
    bool Read(int category, int index);
    bool Apply(int category, int index, bool enable);

    // Categories 4-6 ship as .reg bodies; applying one costs a single reg.exe pass.
    bool IsRegPack(int category);
    bool ApplyCategory(int category, const bool* states, int count);
}
