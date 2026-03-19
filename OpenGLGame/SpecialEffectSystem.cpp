#include "pch.h"
#include "SpecialEffectSystem.h"

EffectBurst SpecialEffectSystem::makeBurst(const vec2& center, const vec4& color, float cellSize, float scale, float duration)
{
    return {
        center,
        vec2(cellSize * scale),
        vec4(color.r, color.g, color.b, 0.65f),
        0.0f,
        duration
    };
}

EffectBurst SpecialEffectSystem::makeBurstForTile(const vec2& center, const Tile& tile, float cellSize, float duration)
{
    if (tile.special == SpecialType::ColorBomb)
    {
        return makeBurst(center, vec4(0.98f, 0.98f, 0.98f, 1.0f), cellSize, 3.0f, duration);
    }
    if (tile.special == SpecialType::Bomb)
    {
        return makeBurst(center, vec4(0.95f, 0.95f, 0.95f, 1.0f), cellSize, 2.4f, duration);
    }
    if (tile.special == SpecialType::RowClear)
    {
        return makeBurst(center, vec4(0.95f, 0.95f, 0.95f, 1.0f), cellSize, 1.8f, duration);
    }
    return makeBurst(center, vec4(1.0f), cellSize, 1.15f, duration);
}
