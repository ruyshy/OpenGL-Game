#include "pch.h"
#include "GameplayState.h"

#include "InputState.h"

namespace
{
    int clampToRange(int value, int minimum, int maximum)
    {
        return (std::max)(minimum, (std::min)(value, maximum));
    }
}

void GameplayState::reset()
{
    _phase = GameplayPhase::Ready;
    _cursor = { 0, 0 };
    _score = 0;
    _moves = 0;
    _elapsedTime = 0.0;
    _statusText = "Press arrow keys or WASD to move. Enter to act. P to pause.";
}

void GameplayState::update(const InputState& input, double deltaTime)
{
    _elapsedTime += deltaTime;

    if (input.wasKeyPressed(GLFW_KEY_P))
    {
        togglePause();
    }

    if (_phase == GameplayPhase::Paused)
    {
        return;
    }

    if (_phase == GameplayPhase::Ready)
    {
        _phase = GameplayPhase::Playing;
        _statusText = "Gameplay running. Esc returns to title.";
    }

    if (input.wasKeyPressed(GLFW_KEY_LEFT) || input.wasKeyPressed(GLFW_KEY_A))
    {
        moveCursor(-1, 0);
    }
    if (input.wasKeyPressed(GLFW_KEY_RIGHT) || input.wasKeyPressed(GLFW_KEY_D))
    {
        moveCursor(1, 0);
    }
    if (input.wasKeyPressed(GLFW_KEY_UP) || input.wasKeyPressed(GLFW_KEY_W))
    {
        moveCursor(0, -1);
    }
    if (input.wasKeyPressed(GLFW_KEY_DOWN) || input.wasKeyPressed(GLFW_KEY_S))
    {
        moveCursor(0, 1);
    }

    if (input.wasKeyPressed(GLFW_KEY_ENTER) || input.wasKeyPressed(GLFW_KEY_SPACE))
    {
        advanceTurn();
    }

    if (_moves >= 10)
    {
        _phase = GameplayPhase::Completed;
        _statusText = "Prototype round complete. Press R to restart or Esc to leave.";
    }

    if (input.wasKeyPressed(GLFW_KEY_R))
    {
        reset();
    }
}

GameplayPhase GameplayState::getPhase() const
{
    return _phase;
}

GridPosition GameplayState::getCursor() const
{
    return _cursor;
}

int GameplayState::getScore() const
{
    return _score;
}

int GameplayState::getMoves() const
{
    return _moves;
}

double GameplayState::getElapsedTime() const
{
    return _elapsedTime;
}

const string& GameplayState::getStatusText() const
{
    return _statusText;
}

void GameplayState::moveCursor(int deltaX, int deltaY)
{
    if (_phase == GameplayPhase::Completed)
    {
        return;
    }

    _cursor.x = clampToRange(_cursor.x + deltaX, 0, GridWidth - 1);
    _cursor.y = clampToRange(_cursor.y + deltaY, 0, GridHeight - 1);
    _statusText = "Cursor moved to (" + std::to_string(_cursor.x) + ", " + std::to_string(_cursor.y) + ").";
}

void GameplayState::togglePause()
{
    if (_phase == GameplayPhase::Completed)
    {
        return;
    }

    if (_phase == GameplayPhase::Paused)
    {
        _phase = GameplayPhase::Playing;
        _statusText = "Gameplay resumed.";
    }
    else
    {
        _phase = GameplayPhase::Paused;
        _statusText = "Gameplay paused. Press P to continue.";
    }
}

void GameplayState::advanceTurn()
{
    if (_phase != GameplayPhase::Playing)
    {
        return;
    }

    ++_moves;
    _score += 100 + (_cursor.x * 10) + (_cursor.y * 5);
    _statusText = "Action resolved at (" + std::to_string(_cursor.x) + ", " + std::to_string(_cursor.y) + ").";
}
