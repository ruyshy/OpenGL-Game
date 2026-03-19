#pragma once

#ifndef SHUFFLERESOLVER_H_
#define SHUFFLERESOLVER_H_

#include "PuzzleTypes.h"

// 플레이 가능한 이동이 남아 있는지 검사하고,
// 없다면 셔플로 다시 살리는 역할을 담당한다.
class ShuffleResolver
{
public:
    static bool hasAvailableMove(PuzzleGrid tiles);
    static void shuffleUntilPlayable(PuzzleGrid& tiles, mt19937& rng);
};

#endif // !SHUFFLERESOLVER_H_
