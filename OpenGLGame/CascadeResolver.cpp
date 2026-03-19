#include "pch.h"
#include "CascadeResolver.h"
#include "PuzzleRenderer.h"
#include "PuzzleRuleEngine.h"

MatchResolution CascadeResolver::findMatches(const PuzzleGrid& tiles, const Cell& swapSource, const Cell& swapTarget)
{
    return PuzzleRuleEngine::findMatches(tiles, swapSource, swapTarget);
}

ExpandedClearResult CascadeResolver::expandClears(const PuzzleGrid& tiles, const MatchResolution& resolution)
{
    return PuzzleRuleEngine::expandSpecialClears(tiles, resolution);
}

CascadeFallResult CascadeResolver::buildFall(
    const PuzzleGrid& tiles,
    mt19937& rng,
    PuzzleRenderer& renderer,
    float minimumFallDuration,
    float fallDurationPerCell)
{
    CascadeFallResult result;
    PuzzleGrid originalTiles = tiles;

    for (int column = 0; column < PuzzleColumnCount; ++column)
    {
        // 아래쪽부터 차곡차곡 다시 쓰는 방식으로 낙하 후 보드를 계산한다.
        int writeRow = 0;
        for (int row = 0; row < PuzzleRowCount; ++row)
        {
            const Tile tile = originalTiles[row][column];
            if (PuzzleRuleEngine::isEmpty(tile))
            {
                continue;
            }

            result.nextTiles[writeRow][column] = tile;
            if (writeRow != row)
            {
                // 떨어진 칸 수에 비례해 낙하 시간을 늘리되,
                // 너무 짧아지면 보이지 않으므로 최소 시간을 보장한다.
                const float duration = std::max(minimumFallDuration, static_cast<float>(row - writeRow) * fallDurationPerCell);
                result.falls.push_back({ tile, renderer.getCellPosition({ column, row }), renderer.getCellPosition({ column, writeRow }), 0.0f, duration, false });
            }
            ++writeRow;
        }

        for (int row = writeRow; row < PuzzleRowCount; ++row)
        {
            // 위쪽 바깥에서 새 타일이 내려오는 것처럼 보이도록 시작 좌표를 보드 위로 잡는다.
            const Tile tile = PuzzleRuleEngine::randomTile(rng);
            result.nextTiles[row][column] = tile;
            const int spawnRow = PuzzleRowCount + (row - writeRow);
            const float duration = std::max(minimumFallDuration, static_cast<float>(spawnRow - row) * fallDurationPerCell);
            result.falls.push_back({ tile, renderer.getCellPosition({ column, spawnRow }), renderer.getCellPosition({ column, row }), 0.0f, duration, false });
        }
    }

    return result;
}
