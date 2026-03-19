#pragma once

#ifndef MATCHFACTORY_H_
#define MATCHFACTORY_H_

#include "PuzzleTypes.h"

// 매치 결과 객체를 어디에서 만들어야 하는지 감추는 팩토리다.
// PuzzleBoard가 규칙 엔진 세부 구현을 직접 알지 않도록 중간 레이어 역할을 한다.
class MatchFactory
{
public:
    static MatchResolution createSpecialSwapMatch(const PuzzleGrid& tiles, const Cell& first, const Cell& second);
    static MatchResolution createBoardMatch(const PuzzleGrid& tiles, const Cell& swapSource, const Cell& swapTarget);
};

#endif // !MATCHFACTORY_H_
