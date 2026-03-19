#pragma once

#ifndef BOARDSTATEMACHINE_H_
#define BOARDSTATEMACHINE_H_

#include "PuzzleTypes.h"

// 보드 상태 전이 규칙만 모아 둔 얇은 헬퍼다.
// 지금은 단순 매핑 수준이지만, 상태 전이가 늘어나도 호출부를 깔끔하게 유지할 수 있다.
class BoardStateMachine
{
public:
    static BoardState stateForSwapResult(bool validSwap);
    static BoardState onInvalidSwapForwardFinished();
    static BoardState onInvalidSwapBackFinished();
    static BoardState onFallsStarted();
    static BoardState onClearStarted();
    static BoardState onWaitingForInput();
};

#endif // !BOARDSTATEMACHINE_H_
