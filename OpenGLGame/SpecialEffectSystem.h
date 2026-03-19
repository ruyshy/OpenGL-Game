#pragma once

#ifndef SPECIALEFFECTSYSTEM_H_
#define SPECIALEFFECTSYSTEM_H_

#include "PuzzleTypes.h"

class SpecialEffectSystem
{
public:
    static EffectBurst makeBurst(const vec2& center, const vec4& color, float cellSize, float scale, float duration);
    static EffectBurst makeBurstForTile(const vec2& center, const Tile& tile, float cellSize, float duration);
};

#endif // !SPECIALEFFECTSYSTEM_H_
