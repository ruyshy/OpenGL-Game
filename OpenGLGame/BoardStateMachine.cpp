#include "pch.h"
#include "BoardStateMachine.h"

BoardState BoardStateMachine::stateForSwapResult(bool validSwap)
{
    // 스왑 성공이면 정방향 스왑 애니메이션 뒤에 매치 확인으로,
    // 실패면 되돌리기용 왕복 애니메이션 상태로 들어간다.
    return validSwap ? BoardState::AnimatingSwap : BoardState::AnimatingInvalidSwapForward;
}

BoardState BoardStateMachine::onInvalidSwapForwardFinished()
{
    return BoardState::AnimatingInvalidSwapBack;
}

BoardState BoardStateMachine::onInvalidSwapBackFinished()
{
    return BoardState::WaitingForInput;
}

BoardState BoardStateMachine::onFallsStarted()
{
    return BoardState::AnimatingFall;
}

BoardState BoardStateMachine::onClearStarted()
{
    return BoardState::AnimatingClear;
}

BoardState BoardStateMachine::onWaitingForInput()
{
    return BoardState::WaitingForInput;
}
