#include "pch.h"
#include "MatchFactory.h"
#include "CascadeResolver.h"
#include "SpecialComboResolver.h"

MatchResolution MatchFactory::createSpecialSwapMatch(const PuzzleGrid& tiles, const Cell& first, const Cell& second)
{
    // 특수 블록끼리의 조합은 일반 3매치 규칙과 별개이므로 전용 리졸버를 탄다.
    return SpecialComboResolver::resolveSwap(tiles, first, second);
}

MatchResolution MatchFactory::createBoardMatch(const PuzzleGrid& tiles, const Cell& swapSource, const Cell& swapTarget)
{
    // 일반 매치 탐지와 특수 블록 생성 위치 계산은 연쇄 리졸버에서 처리한다.
    return CascadeResolver::findMatches(tiles, swapSource, swapTarget);
}
