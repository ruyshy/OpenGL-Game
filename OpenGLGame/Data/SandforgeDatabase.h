#pragma once

#ifndef SANDFORGE_DATABASE_H_
#define SANDFORGE_DATABASE_H_

#include "../Core/SandforgeTypes.h"

struct SandforgeVisualDefinition
{
    string imagePath;
    string animationPath;
    vec2 spriteSize = vec2(48.0f, 48.0f);
};

struct SandforgeUnitDefinition
{
    SandforgeUnitType type = SandforgeUnitType::Worker;
    string displayName;
    int metalCost = 0;
    int energyCost = 0;
    float maxHp = 0.0f;
    float damage = 0.0f;
    float structureDamage = 0.0f;
    float attackCooldown = 0.0f;
    float attackRange = 0.0f;
    float moveSpeed = 0.0f;
    float productionTime = 0.0f;
    bool canCapture = false;
    bool canBuild = false;
    bool isCombatUnit = false;
    bool preferStructureTarget = false;
    SandforgeDamageType damageType = SandforgeDamageType::Normal;
    SandforgeArmorType armorType = SandforgeArmorType::Light;
    SandforgeVisualDefinition visuals;
};

struct SandforgeBuildingDefinition
{
    SandforgeBuildingType type = SandforgeBuildingType::HQ;
    string displayName;
    int metalCost = 0;
    int energyCost = 0;
    float maxHp = 0.0f;
    float buildTime = 0.0f;
    bool isMainBase = false;
    bool isDefeatCondition = false;
    bool isCombatStructure = false;
    float nodeIncomeMultiplier = 1.0f;
    float nodeBonusRadius = 0.0f;
    float attackDamage = 0.0f;
    float attackRange = 0.0f;
    float attackCooldown = 0.0f;
    vector<SandforgeUnitType> producibleUnits;
    SandforgeVisualDefinition visuals;
};

struct SandforgeNodeDefinition
{
    string displayName;
    SandforgeResourceType resourceType = SandforgeResourceType::Metal;
    float captureTime = 5.0f;
    int incomeAmount = 0;
    float incomeInterval = 5.0f;
    SandforgeVisualDefinition visuals;
};

struct SandforgeMatchRules
{
    int startMetal = 300;
    int startEnergy = 50;
    float timeLimitSeconds = 720.0f;
    int startingWorkerCount = 2;
};

class SandforgeDatabase
{
public:
    static const SandforgeUnitDefinition& getUnit(SandforgeUnitType type);
    static const SandforgeBuildingDefinition& getBuilding(SandforgeBuildingType type);
    static const SandforgeNodeDefinition& getNode(SandforgeResourceType type);
    static const SandforgeMatchRules& getMatchRules();
};

#endif // !SANDFORGE_DATABASE_H_
