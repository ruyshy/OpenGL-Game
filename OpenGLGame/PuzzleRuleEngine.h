#pragma once

#ifndef PUZZLERULEENGINE_H_
#define PUZZLERULEENGINE_H_

#include "PuzzleTypes.h"

// 퍼즐 규칙의 핵심 계산을 모아 둔 정적 유틸리티다.
// "무슨 타일이 생성되는가", "어떤 매치가 성립하는가", "특수 블록이 어디까지 퍼지는가"를 담당한다.
class PuzzleRuleEngine
{
public:
    static Tile generateTileForPosition(const PuzzleGrid& tiles, int row, int column, mt19937& rng);
    static Tile randomTile(mt19937& rng);
    static bool isEmpty(const Tile& tile);
    static bool isInsideBoard(const Cell& cell);
    static bool areAdjacent(const Cell& lhs, const Cell& rhs);
    static MatchResolution buildSpecialSwapResolution(const PuzzleGrid& tiles, const Cell& first, const Cell& second);
    static MatchResolution findMatches(const PuzzleGrid& tiles, const Cell& swapSource, const Cell& swapTarget);
    static ExpandedClearResult expandSpecialClears(const PuzzleGrid& tiles, const MatchResolution& resolution);
};

#endif // !PUZZLERULEENGINE_H_
