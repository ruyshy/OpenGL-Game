#pragma once

#ifndef GAMEPLAYSTATE_H_
#define GAMEPLAYSTATE_H_

#include <string>

class InputState;

enum class GameplayPhase
{
    Ready,
    Playing,
    Paused,
    Completed
};

struct GridPosition
{
    int x = 0;
    int y = 0;
};

class GameplayState
{
public:
    void reset();
    void update(const InputState& input, double deltaTime);

    GameplayPhase getPhase() const;
    GridPosition getCursor() const;
    int getScore() const;
    int getMoves() const;
    double getElapsedTime() const;
    const string& getStatusText() const;

private:
    void moveCursor(int deltaX, int deltaY);
    void togglePause();
    void advanceTurn();

private:
    static constexpr int GridWidth = 6;
    static constexpr int GridHeight = 6;

    GameplayPhase _phase = GameplayPhase::Ready;
    GridPosition _cursor{ 0, 0 };
    int _score = 0;
    int _moves = 0;
    double _elapsedTime = 0.0;
    string _statusText = "Press arrow keys or WASD to move. Enter to act. P to pause.";
};

#endif // !GAMEPLAYSTATE_H_
