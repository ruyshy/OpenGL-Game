#pragma once

#ifndef COMBOSYSTEM_H_
#define COMBOSYSTEM_H_

class ComboSystem
{
public:
    static int scoreForClear(int clearedCount, int combo);
    static int scoreForSpawns(int spawnCount);
    static float timeBonusForClear(int combo);
};

#endif // !COMBOSYSTEM_H_
