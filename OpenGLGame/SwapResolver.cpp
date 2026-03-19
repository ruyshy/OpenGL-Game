#include "pch.h"
#include "SwapResolver.h"
#include "PuzzleRuleEngine.h"

void SwapResolver::applySwap(PuzzleGrid& tiles, const Cell& first, const Cell& second)
{
    std::swap(tiles[first.row][first.column], tiles[second.row][second.column]);
}

bool SwapResolver::createsMatch(PuzzleGrid& tiles, const Cell& first, const Cell& second)
{
    // 유효성 검사는 실제 스왑을 가정해서 매치를 찾은 뒤 다시 되돌리는 방식이다.
    applySwap(tiles, first, second);
    const MatchResolution resolution = PuzzleRuleEngine::findMatches(tiles, first, second);
    applySwap(tiles, first, second);

    return any_of(resolution.cells.begin(), resolution.cells.end(), [&](const Cell& matchCell)
    {
        return matchCell == first || matchCell == second;
    });
}
