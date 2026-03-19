#include "pch.h"
#include "ShuffleResolver.h"
#include "PuzzleRuleEngine.h"

namespace
{
    bool createsMatchAtSwap(PuzzleGrid& tiles, const Cell& first, const Cell& second)
    {
        // 실제 보드를 바꾸지 않기 위해 스왑 후 검사하고 다시 원복한다.
        std::swap(tiles[first.row][first.column], tiles[second.row][second.column]);
        const MatchResolution resolution = PuzzleRuleEngine::findMatches(tiles, first, second);
        std::swap(tiles[first.row][first.column], tiles[second.row][second.column]);

        return any_of(resolution.cells.begin(), resolution.cells.end(), [&](const Cell& matchCell)
        {
            return matchCell == first || matchCell == second;
        });
    }
}

bool ShuffleResolver::hasAvailableMove(PuzzleGrid tiles)
{
    // 모든 칸에서 우측/상단 이웃만 검사해도 전체 가능한 스왑을 중복 없이 확인할 수 있다.
    for (int row = 0; row < PuzzleRowCount; ++row)
    {
        for (int column = 0; column < PuzzleColumnCount; ++column)
        {
            const Cell cell{ column, row };
            const array<Cell, 2> neighbors = {
                Cell{ column + 1, row },
                Cell{ column, row + 1 }
            };

            for (const Cell& neighbor : neighbors)
            {
                if (!PuzzleRuleEngine::isInsideBoard(neighbor))
                {
                    continue;
                }

                if (createsMatchAtSwap(tiles, cell, neighbor))
                {
                    return true;
                }
            }
        }
    }

    return false;
}

void ShuffleResolver::shuffleUntilPlayable(PuzzleGrid& tiles, mt19937& rng)
{
    vector<Tile> pool;
    pool.reserve(PuzzleColumnCount * PuzzleRowCount);

    // 너무 오래 루프에 머무르지 않도록 시도 횟수 상한을 둔다.
    for (int attempt = 0; attempt < 64; ++attempt)
    {
        pool.clear();
        for (int row = 0; row < PuzzleRowCount; ++row)
        {
            for (int column = 0; column < PuzzleColumnCount; ++column)
            {
                pool.push_back(tiles[row][column]);
            }
        }

        shuffle(pool.begin(), pool.end(), rng);

        int index = 0;
        for (int row = 0; row < PuzzleRowCount; ++row)
        {
            for (int column = 0; column < PuzzleColumnCount; ++column)
            {
                tiles[row][column] = pool[index++];
            }
        }

        const MatchResolution resolution = PuzzleRuleEngine::findMatches(tiles, {}, {});
        if (resolution.cells.empty() && hasAvailableMove(tiles))
        {
            // 셔플 결과는 "시작하자마자 터지지 않고", "둘 수 있는 수가 있는" 상태여야 한다.
            return;
        }
    }
}
