#pragma once

#ifndef SANDFORGE_TYPES_H_
#define SANDFORGE_TYPES_H_

#include "../pch.h"

using SandforgeEntityId = uint32_t;
using SandforgePlayerId = uint32_t;

struct SandforgeVec2
{
    float x = 0.0f;
    float y = 0.0f;
};

enum class SandforgeEntityType : uint8_t
{
    None,
    Unit,
    Building,
    ResourceNode
};

enum class SandforgeUnitType : uint8_t
{
    Worker,
    Soldier,
    Defender,
    Raider,
    RangerMech,
    SiegeUnit
};

enum class SandforgeBuildingType : uint8_t
{
    HQ,
    Barracks,
    Factory,
    NodeHub,
    DefenseTower
};

enum class SandforgeResourceType : uint8_t
{
    Metal,
    Energy
};

enum class SandforgeDamageType : uint8_t
{
    Normal,
    Piercing,
    Siege
};

enum class SandforgeArmorType : uint8_t
{
    Light,
    Armored,
    Structure
};

enum class SandforgeUnitState : uint8_t
{
    Idle,
    Move,
    Capture,
    Gather,
    ReturnResource,
    Advance,
    Attack,
    Dead
};

enum class SandforgeMatchEndReason : uint8_t
{
    None,
    HQDestroyed,
    Timeout
};

inline float sandforgeDistanceSquared(const SandforgeVec2& a, const SandforgeVec2& b)
{
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    return (dx * dx) + (dy * dy);
}

inline vec2 sandforgeToGlm(const SandforgeVec2& value)
{
    return vec2(value.x, value.y);
}

inline SandforgeVec2 sandforgeFromGlm(const vec2& value)
{
    return SandforgeVec2{ value.x, value.y };
}

#endif // !SANDFORGE_TYPES_H_
