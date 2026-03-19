#include "pch.h"
#include "SpecialComboResolver.h"
#include "PuzzleRuleEngine.h"

MatchResolution SpecialComboResolver::resolveSwap(const PuzzleGrid& tiles, const Cell& first, const Cell& second)
{
    // 실제 세부 조합 규칙은 룰 엔진에 있고,
    // 이 클래스는 보드 파이프라인에서 "특수 조합 판정" 역할을 분리하기 위한 얇은 레이어다.
    return PuzzleRuleEngine::buildSpecialSwapResolution(tiles, first, second);
}
