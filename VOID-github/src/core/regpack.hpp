#pragma once

// The wezytweak .reg pack, embedded as text. Nothing is read from disk: a tweak
// writes its .reg body to a temp file, hands it to reg.exe, then deletes it.
namespace regpack
{
    // Writes `text` as a UTF-16LE .reg file in %TEMP%, imports it, removes it.
    bool Import(const char* text);

    // Merges several bodies into one .reg so a whole category costs a single import.
    bool ImportMany(const char* const* bodies, int count);

    // Embedded bodies for tweak categories 4 (Games), 5 (FiveM), 6 (Delay).
    const char* Body(int cat, int idx, bool enable);

    // Built at runtime from the chosen size; gb <= 0 restores the Windows default.
    bool ApplyRamProfile(int gb);
}
