#include "pch.h"
#include "TetrisInputController.h"

#include "OpenGLWindow.h"
#include "TetrisGame.h"

void TetrisInputController::reset()
{
    // 누르고 있던 키의 반복 상태를 새 게임/일시정지 전환 시점에 정리한다.
    moveAccumulator_ = 0.0;
    softDropAccumulator_ = 0.0;
}

bool TetrisInputController::process(OpenGLWindow& window, TetrisGame& game)
{
    // 종료/재시작/일시정지는 게임 규칙보다 우선 처리해서
    // 현재 프레임의 다른 입력과 섞여도 결과가 예측 가능하도록 만든다.
    if (window.keyPressedOnce(GLFW_KEY_ESCAPE))
    {
        window.closeWindow();
        return false;
    }

    if (window.keyPressedOnce(GLFW_KEY_R))
    {
        game.reset();
        reset();
        return true;
    }

    if (window.keyPressedOnce(GLFW_KEY_P))
    {
        game.togglePause();
        reset();
        return true;
    }

    if (game.isGameOver())
    {
        return true;
    }

    if (game.isPaused())
    {
        return true;
    }

    const double delta = window.getTimeDelta();

    // 좌우 이동은 "한 번 눌렀을 때 즉시 1칸" + "누르고 있으면 일정 주기로 반복" 구조다.
    // 이렇게 분리하면 키보드 OS 반복 속도와 무관하게 항상 같은 감도로 조작된다.
    moveAccumulator_ += delta;

    if (window.keyPressedOnce(GLFW_KEY_UP) || window.keyPressedOnce(GLFW_KEY_X) || window.keyPressedOnce(GLFW_KEY_SPACE))
    {
        game.rotatePieceClockwise();
    }

    if (window.keyPressedOnce(GLFW_KEY_Z))
    {
        game.rotatePieceCounterClockwise();
    }

    if (window.keyPressedOnce(GLFW_KEY_LEFT_SHIFT) || window.keyPressedOnce(GLFW_KEY_H))
    {
        game.holdCurrentPiece();
        return true;
    }

    if (window.keyPressedOnce(GLFW_KEY_LEFT))
    {
        game.movePiece(-1, 0);
        moveAccumulator_ = 0.0;
    }
    else if (window.keyPressedOnce(GLFW_KEY_RIGHT))
    {
        game.movePiece(1, 0);
        moveAccumulator_ = 0.0;
    }
    else if (moveAccumulator_ >= 0.12)
    {
        // 반복 이동은 입력 계층에서만 처리하고, 게임 로직은 "지금 1칸 이동 가능한가"
        // 같은 순수 규칙 판단만 담당하게 분리한다.
        if (window.keyPressed(GLFW_KEY_LEFT))
        {
            game.movePiece(-1, 0);
            moveAccumulator_ = 0.0;
        }
        else if (window.keyPressed(GLFW_KEY_RIGHT))
        {
            game.movePiece(1, 0);
            moveAccumulator_ = 0.0;
        }
    }

    if (window.keyPressedOnce(GLFW_KEY_C))
    {
        game.hardDrop();
        return true;
    }

    if (window.keyPressed(GLFW_KEY_DOWN))
    {
        softDropAccumulator_ += delta;
        if (softDropAccumulator_ >= 0.03)
        {
            // 소프트 드롭은 기본 중력보다 훨씬 빠르지만, 하드 드롭처럼 즉시 고정되지는 않게
            // 별도 반복 주기를 둔다. 그래서 "빠르게 내리기"와 "즉시 착지" 감각이 분리된다.
            game.softDrop();
            softDropAccumulator_ = 0.0;
        }
    }
    else
    {
        softDropAccumulator_ = 0.0;
    }

    return true;
}
