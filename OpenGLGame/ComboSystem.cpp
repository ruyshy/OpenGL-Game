#include "pch.h"
#include "ComboSystem.h"

int ComboSystem::scoreForClear(int clearedCount, int combo)
{
    return clearedCount * 10 * std::max(1, combo);
}

int ComboSystem::scoreForSpawns(int spawnCount)
{
    return spawnCount * 40;
}

float ComboSystem::timeBonusForClear(int combo)
{
    return 0.6f * static_cast<float>(std::max(1, combo));
}
