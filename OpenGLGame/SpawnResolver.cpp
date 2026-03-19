#include "pch.h"
#include "SpawnResolver.h"
#include "PuzzleRuleEngine.h"

void SpawnResolver::buildInitialBoard(PuzzleGrid& tiles, mt19937& rng)
{
    // 좌상단부터 순서대로 타일을 채우면서,
    // 직전에 놓인 타일 때문에 즉시 3매치가 생기지 않도록 생성한다.
    for (int row = 0; row < PuzzleRowCount; ++row)
    {
        for (int column = 0; column < PuzzleColumnCount; ++column)
        {
            tiles[row][column] = PuzzleRuleEngine::generateTileForPosition(tiles, row, column, rng);
        }
    }
}

Tile SpawnResolver::randomRefillTile(mt19937& rng)
{
    // 리필은 개별 칸 단위의 완전 무작위 타일만 필요하다.
    return PuzzleRuleEngine::randomTile(rng);
}
