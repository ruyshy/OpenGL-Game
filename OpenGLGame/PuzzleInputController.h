#pragma once

#ifndef PUZZLEINPUTCONTROLLER_H_
#define PUZZLEINPUTCONTROLLER_H_

#include "PuzzleTypes.h"

enum class InputActionType
{
    None,
    Select,
    Deselect,
    Swap
};

struct InputAction
{
    InputActionType type = InputActionType::None;
    Cell target{};
};

// 클릭 한 번을 "선택/해제/스왑" 같은 의미 있는 행동으로 바꿔 주는 입력 해석기다.
class PuzzleInputController
{
public:
    static InputAction processClick(bool hasSelection, const Cell& selectedCell, const Cell& clickedCell);
};

#endif // !PUZZLEINPUTCONTROLLER_H_
