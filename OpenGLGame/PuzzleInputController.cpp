#include "pch.h"
#include "PuzzleInputController.h"
#include "PuzzleRuleEngine.h"

InputAction PuzzleInputController::processClick(bool hasSelection, const Cell& selectedCell, const Cell& clickedCell)
{
    if (!hasSelection)
    {
        // 아직 선택된 칸이 없으면 첫 클릭은 무조건 선택이다.
        return { InputActionType::Select, clickedCell };
    }

    if (selectedCell == clickedCell)
    {
        // 같은 칸을 다시 누르면 선택을 해제해 조작 실수를 줄인다.
        return { InputActionType::Deselect, clickedCell };
    }

    if (!PuzzleRuleEngine::areAdjacent(selectedCell, clickedCell))
    {
        // 인접하지 않다면 스왑이 아니라 선택 대상을 바꾸는 것으로 해석한다.
        return { InputActionType::Select, clickedCell };
    }

    // 인접한 두 칸이면 실제 스왑 시도로 넘긴다.
    return { InputActionType::Swap, clickedCell };
}
