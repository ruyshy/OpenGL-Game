#pragma once

#ifndef SPECIALCOMBORESOLVER_H_
#define SPECIALCOMBORESOLVER_H_

#include "PuzzleTypes.h"

// 특수 블록끼리, 또는 특수 블록과 일반 블록의 조합 규칙만 담당한다.
class SpecialComboResolver
{
public:
    static MatchResolution resolveSwap(const PuzzleGrid& tiles, const Cell& first, const Cell& second);
};

#endif // !SPECIALCOMBORESOLVER_H_
