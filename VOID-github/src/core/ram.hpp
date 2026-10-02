#pragma once

// SvcHostSplitThresholdInKB tuning (wezytweak "Ram Optimization" .reg pack).
// Raising the threshold to the installed RAM size makes Windows group services
// into fewer svchost.exe processes instead of splitting one per service.
namespace ram
{
    int         PresetCount();
    int         PresetGB(int index);       // 0 = restore Windows default
    const char* PresetName(int index);

    int  DetectedGB();                     // installed RAM rounded to nearest preset
    int  CurrentPreset();                  // preset matching the live registry value, else -1
    bool Apply(int index);
}
