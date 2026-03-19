#include "pch.h"
#include "PuzzleRuleEngine.h"

Tile PuzzleRuleEngine::generateTileForPosition(const PuzzleGrid& tiles, int row, int column, mt19937& rng)
{
    // 현재 칸의 왼쪽/아래쪽만 봐도 이미 채워진 타일 기준으로
    // 즉시 3매치가 생기는지를 판단할 수 있다.
    vector<int> candidates;
    candidates.reserve(PuzzleTileTypeCount);

    for (int color = 0; color < PuzzleTileTypeCount; ++color)
    {
        const bool createsHorizontalMatch = column >= 2
            && tiles[row][column - 1].color == color
            && tiles[row][column - 2].color == color;

        const bool createsVerticalMatch = row >= 2
            && tiles[row - 1][column].color == color
            && tiles[row - 2][column].color == color;

        if (!createsHorizontalMatch && !createsVerticalMatch)
        {
            candidates.push_back(color);
        }
    }

    uniform_int_distribution<int> distribution(0, static_cast<int>(candidates.size()) - 1);
    return { candidates[distribution(rng)], SpecialType::None };
}

Tile PuzzleRuleEngine::randomTile(mt19937& rng)
{
    // 특수 블록 없이 순수 색상만 무작위로 뽑는다.
    uniform_int_distribution<int> distribution(0, PuzzleTileTypeCount - 1);
    return { distribution(rng), SpecialType::None };
}

bool PuzzleRuleEngine::isEmpty(const Tile& tile)
{
    return tile.color < 0;
}

bool PuzzleRuleEngine::isInsideBoard(const Cell& cell)
{
    return cell.column >= 0 && cell.column < PuzzleColumnCount && cell.row >= 0 && cell.row < PuzzleRowCount;
}

bool PuzzleRuleEngine::areAdjacent(const Cell& lhs, const Cell& rhs)
{
    return abs(lhs.column - rhs.column) + abs(lhs.row - rhs.row) == 1;
}

MatchResolution PuzzleRuleEngine::buildSpecialSwapResolution(const PuzzleGrid& tiles, const Cell& first, const Cell& second)
{
    MatchResolution resolution;
    const Tile& firstTile = tiles[first.row][first.column];
    const Tile& secondTile = tiles[second.row][second.column];

    set<pair<int, int>> marked;

    const auto markCell = [&](int column, int row)
    {
        // 보드 밖 좌표는 무시하고, 중복 표시는 set으로 자동 제거한다.
        if (column >= 0 && column < PuzzleColumnCount && row >= 0 && row < PuzzleRowCount)
        {
            marked.insert({ column, row });
        }
    };

    const auto addCross = [&](const Cell& center)
    {
        // 줄 제거 + 폭탄 같은 조합에서 쓰는 십자 범위 마킹이다.
        for (int column = 0; column < PuzzleColumnCount; ++column)
        {
            markCell(column, center.row);
        }

        for (int row = 0; row < PuzzleRowCount; ++row)
        {
            markCell(center.column, row);
        }
    };

    const bool isRowBombCombo =
        (firstTile.special == SpecialType::RowClear && secondTile.special == SpecialType::Bomb)
        || (firstTile.special == SpecialType::Bomb && secondTile.special == SpecialType::RowClear);

    const bool isRowRowCombo =
        firstTile.special == SpecialType::RowClear && secondTile.special == SpecialType::RowClear;

    const bool isBombBombCombo =
        firstTile.special == SpecialType::Bomb && secondTile.special == SpecialType::Bomb;

    const bool isColorBombColorBombCombo =
        firstTile.special == SpecialType::ColorBomb && secondTile.special == SpecialType::ColorBomb;

    const bool isColorBombBombCombo =
        (firstTile.special == SpecialType::ColorBomb && secondTile.special == SpecialType::Bomb)
        || (firstTile.special == SpecialType::Bomb && secondTile.special == SpecialType::ColorBomb);

    const bool isColorBombRowCombo =
        (firstTile.special == SpecialType::ColorBomb && secondTile.special == SpecialType::RowClear)
        || (firstTile.special == SpecialType::RowClear && secondTile.special == SpecialType::ColorBomb);

    const bool isColorBombNormalCombo =
        (firstTile.special == SpecialType::ColorBomb && secondTile.special == SpecialType::None)
        || (firstTile.special == SpecialType::None && secondTile.special == SpecialType::ColorBomb);

    const bool isBombNormalCombo =
        (firstTile.special == SpecialType::Bomb && secondTile.special == SpecialType::None)
        || (firstTile.special == SpecialType::None && secondTile.special == SpecialType::Bomb);

    const bool isRowNormalCombo =
        (firstTile.special == SpecialType::RowClear && secondTile.special == SpecialType::None)
        || (firstTile.special == SpecialType::None && secondTile.special == SpecialType::RowClear);

    if (isColorBombColorBombCombo)
    {
        // 컬러 폭탄끼리는 보드 전체 제거로 처리한다.
        for (int row = 0; row < PuzzleRowCount; ++row)
        {
            for (int column = 0; column < PuzzleColumnCount; ++column)
            {
                markCell(column, row);
            }
        }
    }
    else if (isColorBombBombCombo || isColorBombRowCombo || isColorBombNormalCombo)
    {
        // 컬러 폭탄 + 다른 타일은 "해당 색 전체"를 기준으로 확장된다.
        // bomb/row 조합일 때는 그 색 타일들을 임시 특수 블록처럼 발동시키기 위해
        // triggeredSpecials에 변환 결과를 함께 담아 둔다.
        const Tile sourceTile = firstTile.special == SpecialType::ColorBomb ? secondTile : firstTile;
        const int targetColor = sourceTile.color;
        const SpecialType transformedSpecial =
            isColorBombBombCombo ? SpecialType::Bomb :
            (isColorBombRowCombo ? SpecialType::RowClear : SpecialType::None);

        for (int row = 0; row < PuzzleRowCount; ++row)
        {
            for (int column = 0; column < PuzzleColumnCount; ++column)
            {
                if (tiles[row][column].color == targetColor)
                {
                    markCell(column, row);
                    if (transformedSpecial != SpecialType::None)
                    {
                        resolution.triggeredSpecials.push_back({ { column, row }, Tile{ targetColor, transformedSpecial } });
                    }
                }
            }
        }
    }
    else if (isRowBombCombo)
    {
        addCross(second);
    }
    else if (isRowRowCombo)
    {
        addCross(first);
        addCross(second);
    }
    else if (isBombBombCombo)
    {
        // 폭탄 두 개의 중심을 기준으로 5x5 범위를 터뜨린다.
        const int minColumn = std::min(first.column, second.column);
        const int maxColumn = std::max(first.column, second.column);
        const int minRow = std::min(first.row, second.row);
        const int maxRow = std::max(first.row, second.row);

        const int centerColumn = (minColumn + maxColumn) / 2;
        const int centerRow = (minRow + maxRow) / 2;

        for (int row = centerRow - 2; row <= centerRow + 2; ++row)
        {
            for (int column = centerColumn - 2; column <= centerColumn + 2; ++column)
            {
                markCell(column, row);
            }
        }
    }
    else if (isBombNormalCombo)
    {
        const int targetColor = firstTile.special == SpecialType::Bomb ? secondTile.color : firstTile.color;
        for (int row = 0; row < PuzzleRowCount; ++row)
        {
            for (int column = 0; column < PuzzleColumnCount; ++column)
            {
                if (tiles[row][column].color == targetColor)
                {
                    markCell(column, row);
                }
            }
        }
    }
    else if (isRowNormalCombo)
    {
        const int targetColor = firstTile.special == SpecialType::RowClear ? secondTile.color : firstTile.color;
        for (int row = 0; row < PuzzleRowCount; ++row)
        {
            for (int column = 0; column < PuzzleColumnCount; ++column)
            {
                if (tiles[row][column].color == targetColor)
                {
                    markCell(column, row);
                    resolution.triggeredSpecials.push_back({ { column, row }, Tile{ targetColor, SpecialType::RowClear } });
                }
            }
        }
    }
    else
    {
        // 특별한 조합이 아니라면 빈 결과를 돌려서 일반 매치 검사로 넘긴다.
        return resolution;
    }

    for (const auto& entry : marked)
    {
        resolution.cells.push_back({ entry.first, entry.second });
    }

    return resolution;
}

MatchResolution PuzzleRuleEngine::findMatches(const PuzzleGrid& tiles, const Cell& swapSource, const Cell& swapTarget)
{
    MatchResolution resolution;
    array<array<bool, PuzzleColumnCount>, PuzzleRowCount> marked{};
    array<array<int, PuzzleColumnCount>, PuzzleRowCount> horizontalLen{};
    array<array<int, PuzzleColumnCount>, PuzzleRowCount> verticalLen{};

    // 1차 패스: 가로 연속 구간을 찾아 길이와 마킹 정보를 저장한다.
    for (int row = 0; row < PuzzleRowCount; ++row)
    {
        int start = 0;
        while (start < PuzzleColumnCount)
        {
            const Tile& tile = tiles[row][start];
            int end = start + 1;
            while (end < PuzzleColumnCount && tiles[row][end].color == tile.color && !isEmpty(tile))
            {
                ++end;
            }

            const int length = end - start;
            if (!isEmpty(tile) && length >= 3)
            {
                for (int column = start; column < end; ++column)
                {
                    marked[row][column] = true;
                    horizontalLen[row][column] = length;
                }
            }

            start = end;
        }
    }

    // 2차 패스: 세로 연속 구간도 같은 방식으로 기록한다.
    for (int column = 0; column < PuzzleColumnCount; ++column)
    {
        int start = 0;
        while (start < PuzzleRowCount)
        {
            const Tile& tile = tiles[start][column];
            int end = start + 1;
            while (end < PuzzleRowCount && tiles[end][column].color == tile.color && !isEmpty(tile))
            {
                ++end;
            }

            const int length = end - start;
            if (!isEmpty(tile) && length >= 3)
            {
                for (int row = start; row < end; ++row)
                {
                    marked[row][column] = true;
                    verticalLen[row][column] = length;
                }
            }

            start = end;
        }
    }

    // 3차 패스: 가로/세로 매치가 붙어 있는 칸들을 하나의 컴포넌트로 묶는다.
    // 이렇게 해야 T/L 형태를 한 번에 해석할 수 있다.
    array<array<bool, PuzzleColumnCount>, PuzzleRowCount> visited{};
    for (int row = 0; row < PuzzleRowCount; ++row)
    {
        for (int column = 0; column < PuzzleColumnCount; ++column)
        {
            if (!marked[row][column] || visited[row][column])
            {
                continue;
            }

            vector<Cell> component;
            queue<Cell> pending;
            pending.push({ column, row });
            visited[row][column] = true;
            const int color = tiles[row][column].color;

            while (!pending.empty())
            {
                const Cell current = pending.front();
                pending.pop();
                component.push_back(current);

                const array<Cell, 4> neighbors = {
                    Cell{ current.column + 1, current.row },
                    Cell{ current.column - 1, current.row },
                    Cell{ current.column, current.row + 1 },
                    Cell{ current.column, current.row - 1 }
                };

                for (const Cell& neighbor : neighbors)
                {
                    if (!isInsideBoard(neighbor) || visited[neighbor.row][neighbor.column] || !marked[neighbor.row][neighbor.column])
                    {
                        continue;
                    }

                    if (tiles[neighbor.row][neighbor.column].color != color)
                    {
                        continue;
                    }

                    visited[neighbor.row][neighbor.column] = true;
                    pending.push(neighbor);
                }
            }

            resolution.cells.insert(resolution.cells.end(), component.begin(), component.end());

            // 이번 매치에서 특수 블록이 생성될 기준 칸(anchor)을 고른다.
            // 가능하면 실제 스왑에 참여한 칸을 우선으로 잡아 손맛을 맞춘다.
            Cell anchor = component.front();
            if (find(component.begin(), component.end(), swapTarget) != component.end())
            {
                anchor = swapTarget;
            }
            else if (find(component.begin(), component.end(), swapSource) != component.end())
            {
                anchor = swapSource;
            }

            bool hasCross = false;
            bool hasLengthFive = false;
            bool hasLengthFour = false;
            for (const Cell& cell : component)
            {
                // 가로와 세로가 동시에 겹치는 칸이 있으면 T/L/십자 형태로 보고 폭탄을 만든다.
                if (horizontalLen[cell.row][cell.column] >= 3 && verticalLen[cell.row][cell.column] >= 3)
                {
                    hasCross = true;
                    anchor = cell;
                    break;
                }
                if (horizontalLen[cell.row][cell.column] >= 5 || verticalLen[cell.row][cell.column] >= 5)
                {
                    hasLengthFive = true;
                    anchor = cell;
                }
                if (horizontalLen[cell.row][cell.column] >= 4 || verticalLen[cell.row][cell.column] >= 4)
                {
                    hasLengthFour = true;
                    anchor = cell;
                }
            }

            if (hasCross)
            {
                resolution.spawns.push_back({ anchor, Tile{ color, SpecialType::Bomb } });
            }
            else if (hasLengthFive)
            {
                resolution.spawns.push_back({ anchor, Tile{ color, SpecialType::ColorBomb } });
            }
            else if (hasLengthFour)
            {
                resolution.spawns.push_back({ anchor, Tile{ color, SpecialType::RowClear } });
            }
        }
    }

    sort(resolution.cells.begin(), resolution.cells.end(), [](const Cell& lhs, const Cell& rhs)
    {
        if (lhs.row != rhs.row)
        {
            return lhs.row < rhs.row;
        }
        return lhs.column < rhs.column;
    });
    resolution.cells.erase(unique(resolution.cells.begin(), resolution.cells.end(), [](const Cell& lhs, const Cell& rhs)
    {
        return lhs.row == rhs.row && lhs.column == rhs.column;
    }), resolution.cells.end());

    return resolution;
}

ExpandedClearResult PuzzleRuleEngine::expandSpecialClears(const PuzzleGrid& tiles, const MatchResolution& resolution)
{
    ExpandedClearResult expanded;
    set<pair<int, int>> cellsToClear;
    queue<Cell> pending;
    set<pair<int, int>> processedSpecials;
    map<pair<int, int>, Tile> triggeredOverrides;

    // 먼저 기본 제거 대상과 "이번 제거에서 임시 특수 블록처럼 취급할 칸"을 준비한다.
    for (const Cell& cell : resolution.cells)
    {
        cellsToClear.insert({ cell.column, cell.row });
    }

    for (const SpecialSpawn& triggered : resolution.triggeredSpecials)
    {
        triggeredOverrides[{ triggered.cell.column, triggered.cell.row }] = triggered.tile;
    }

    for (const auto& entry : cellsToClear)
    {
        pending.push({ entry.first, entry.second });
    }

    // 큐를 사용해 특수 블록 연쇄를 너비 우선으로 확장한다.
    while (!pending.empty())
    {
        const Cell cell = pending.front();
        pending.pop();
        Tile tile = tiles[cell.row][cell.column];
        const auto overrideIt = triggeredOverrides.find({ cell.column, cell.row });
        if (overrideIt != triggeredOverrides.end())
        {
            tile = overrideIt->second;
        }
        if (tile.special == SpecialType::None)
        {
            continue;
        }

        const pair<int, int> key{ cell.column, cell.row };
        if (processedSpecials.count(key) != 0)
        {
            continue;
        }
        processedSpecials.insert(key);
        expanded.activatedSpecials.push_back({ cell, tile });

        if (tile.special == SpecialType::RowClear)
        {
            // 줄 제거는 해당 행 전체를 추가 제거 대상으로 넣는다.
            for (int column = 0; column < PuzzleColumnCount; ++column)
            {
                const pair<int, int> expandedCell{ column, cell.row };
                if (cellsToClear.insert(expandedCell).second)
                {
                    pending.push({ column, cell.row });
                }
            }
        }
        else if (tile.special == SpecialType::Bomb)
        {
            // 폭탄은 중심 포함 3x3 범위로 퍼진다.
            for (int row = cell.row - 1; row <= cell.row + 1; ++row)
            {
                for (int column = cell.column - 1; column <= cell.column + 1; ++column)
                {
                    const Cell expandedCell{ column, row };
                    if (!isInsideBoard(expandedCell))
                    {
                        continue;
                    }

                    const pair<int, int> key2{ column, row };
                    if (cellsToClear.insert(key2).second)
                    {
                        pending.push(expandedCell);
                    }
                }
            }
        }
    }

    for (const auto& entry : cellsToClear)
    {
        expanded.clearedCells.push_back({ entry.first, entry.second });
    }

    return expanded;
}
