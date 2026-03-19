#pragma once

#ifndef SWAPRESOLVER_H_
#define SWAPRESOLVER_H_

#include "PuzzleTypes.h"

// 두 칸 교환과 "이 스왑이 유효한가" 판단만 담당하는 작은 모듈이다.
class SwapResolver
{
public:
    static void applySwap(PuzzleGrid& tiles, const Cell& first, const Cell& second);
    static bool createsMatch(PuzzleGrid& tiles, const Cell& first, const Cell& second);
};

#endif // !SWAPRESOLVER_H_
