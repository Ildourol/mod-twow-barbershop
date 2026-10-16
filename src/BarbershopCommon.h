#ifndef MOD_TWOW_BARBERSHOP_COMMON_H
#define MOD_TWOW_BARBERSHOP_COMMON_H

#include "Common.h"
#include "ObjectGuid.h"
#include "Objects/UpdateFields.h"

#include <cstring>
#include <string>
#include <vector>

namespace Barbershop
{
    static constexpr char const* Prefix = "TW_BARBERSHOP";
    static constexpr char const* MessagePrefix = "TW_BARBERSHOP\t";

    // Display ID 4 (Rabbit/Box model) is flashed briefly on the player's unit
    // to force the client to reconstruct and re-render the character model mesh.
    static constexpr uint32 DisplayIdBox = 4;

    enum Customization : uint8
    {
        CUSTOMIZATION_STYLE  = 0,
        CUSTOMIZATION_COLOR  = 1,
        CUSTOMIZATION_FACIAL = 2,
        CUSTOMIZATION_COUNT  = 3
    };

    // Mapping to Player UpdateFields and corresponding byte offsets: { FieldIndex, ByteOffset }
    static constexpr uint32 CustomizationBytes[CUSTOMIZATION_COUNT][2] =
    {
        { PLAYER_BYTES, 2 },   // Style: Hair style
        { PLAYER_BYTES, 3 },   // Color: Hair color
        { PLAYER_BYTES_2, 0 }  // Facial: Facial hair / markings / tusks / earrings / features
    };

    static constexpr char const* PreviewCommands[CUSTOMIZATION_COUNT] =
    {
        "C2S_PREVIEW_STYLE",
        "C2S_PREVIEW_COLOR",
        "C2S_PREVIEW_FACIAL_HAIR"
    };

    enum BarbershopError : uint32
    {
        BARBERSHOP_ERROR_NO_SESSION  = 1,
        BARBERSHOP_ERROR_NO_MONEY    = 2,
        BARBERSHOP_ERROR_IN_SESSION  = 3,
        BARBERSHOP_ERROR_NOT_ALLOWED = 4,
        BARBERSHOP_ERROR_SYSTEM      = 5
    };

    // Available customization choices per race and sex: [Customization][RaceId (0..10)][Gender (0: Male, 1: Female)]
    // Synchronized with Turtle_Barbershop.lua in Turtle WoW client patch-4/patch-9.
    // Race 9 is Goblin, Race 10 is High Elf (RACE_BLOODELF in database / client).
    static constexpr uint8 RaceLimits[CUSTOMIZATION_COUNT][11][2] =
    {
        {   // 0: Hair Style
            { 0, 0 }, { 17, 26 }, { 13, 14 }, { 17, 19 }, { 12, 12 }, { 16, 17 },
            { 13, 12 }, { 12, 12 }, { 10, 10 }, { 14, 17 }, { 16, 20 }
        },
        {   // 1: Hair Color
            { 0, 0 }, { 10, 10 }, { 8, 8 }, { 10, 10 }, { 10, 10 }, { 11, 11 },
            { 9, 9 }, { 10, 10 }, { 10, 10 }, { 5, 5 }, { 16, 16 }
        },
        {   // 2: Facial Hair / Features / Markings / Tusks / Earrings
            { 0, 0 }, { 9, 7 }, { 11, 7 }, { 21, 6 }, { 14, 10 }, { 17, 8 },
            { 9, 7 }, { 12, 7 }, { 14, 10 }, { 10, 5 }, { 11, 11 }
        }
    };

    struct Look
    {
        uint8 values[CUSTOMIZATION_COUNT] = { 0, 0, 0 };

        bool operator==(Look const& other) const
        {
            return std::memcmp(values, other.values, sizeof(values)) == 0;
        }

        bool operator!=(Look const& other) const
        {
            return !(*this == other);
        }
    };

    struct Session
    {
        ObjectGuid chair;
        Look original; // Persisted appearance of the character
        Look preview;  // Live preview rendered solely for the client in chair
    };
}

#endif // MOD_TWOW_BARBERSHOP_COMMON_H
