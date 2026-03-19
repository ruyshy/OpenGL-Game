#pragma once

#ifndef TETRISHUDCONFIG_H_
#define TETRISHUDCONFIG_H_

#include "pch.h"

namespace tetris::hud
{
    inline constexpr float CellPadding = 3.0f;

    inline constexpr std::array<std::array<int, 7>, 10> DigitSegments = { {
        { 1, 1, 1, 1, 1, 1, 0 },
        { 0, 1, 1, 0, 0, 0, 0 },
        { 1, 1, 0, 1, 1, 0, 1 },
        { 1, 1, 1, 1, 0, 0, 1 },
        { 0, 1, 1, 0, 0, 1, 1 },
        { 1, 0, 1, 1, 0, 1, 1 },
        { 1, 0, 1, 1, 1, 1, 1 },
        { 1, 1, 1, 0, 0, 0, 0 },
        { 1, 1, 1, 1, 1, 1, 1 },
        { 1, 1, 1, 1, 0, 1, 1 }
    } };

    inline const std::map<char, std::array<std::string, 5>> Glyphs = {
        { 'A', {"01110","10001","11111","10001","10001"} },
        { 'B', {"11110","10001","11110","10001","11110"} },
        { 'C', {"01111","10000","10000","10000","01111"} },
        { 'D', {"11110","10001","10001","10001","11110"} },
        { 'E', {"11111","10000","11110","10000","11111"} },
        { 'G', {"01111","10000","10111","10001","01110"} },
        { 'H', {"10001","10001","11111","10001","10001"} },
        { 'I', {"11111","00100","00100","00100","11111"} },
        { 'L', {"10000","10000","10000","10000","11111"} },
        { 'M', {"10001","11011","10101","10001","10001"} },
        { 'N', {"10001","11001","10101","10011","10001"} },
        { 'O', {"01110","10001","10001","10001","01110"} },
        { 'P', {"11110","10001","11110","10000","10000"} },
        { 'R', {"11110","10001","11110","10010","10001"} },
        { 'S', {"01111","10000","01110","00001","11110"} },
        { 'T', {"11111","00100","00100","00100","00100"} },
        { 'U', {"10001","10001","10001","10001","01110"} },
        { 'V', {"10001","10001","10001","01010","00100"} },
        { 'X', {"10001","01010","00100","01010","10001"} },
        { 'Y', {"10001","01010","00100","00100","00100"} },
        { ' ', {"00000","00000","00000","00000","00000"} }
    };

    inline constexpr const char* HoldLabel = "HOLD";
    inline constexpr const char* NextLabel = "NEXT";
    inline constexpr const char* ScoreLabel = "SCORE";
    inline constexpr const char* LinesLabel = "LINES";
    inline constexpr const char* LevelLabel = "LEVEL";
    inline constexpr const char* ComboLabel = "COMBO";
    inline constexpr const char* PausedLabel = "PAUSED";
    inline constexpr const char* GameOverLabel = "GAME OVER";
}

#endif // !TETRISHUDCONFIG_H_
