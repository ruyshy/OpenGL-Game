#pragma once

#ifndef CASCADERESOLVER_H_
#define CASCADERESOLVER_H_

#include "PuzzleTypes.h"

struct CascadeFallResult
{
    PuzzleGrid nextTiles{};
    vector<TileAnimation> falls;
};

class PuzzleRenderer;

// 매치 탐지, 특수 확장, 낙하 계산처럼 연쇄 처리에 해당하는 로직을 모은 모듈이다.
class CascadeResolver
{
public:
    static MatchResolution findMatches(const PuzzleGrid& tiles, const Cell& swapSource, const Cell& swapTarget);
    static ExpandedClearResult expandClears(const PuzzleGrid& tiles, const MatchResolution& resolution);
    static CascadeFallResult buildFall(
        const PuzzleGrid& tiles,
        mt19937& rng,
        PuzzleRenderer& renderer,
        float minimumFallDuration,
        float fallDurationPerCell);
};

#endif // !CASCADERESOLVER_H_
