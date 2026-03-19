#pragma once

#ifndef GAMESESSION_H_
#define GAMESESSION_H_

#include "PuzzleTypes.h"

// 점수, 콤보, 제한 시간처럼 "한 판의 진행 정보"를 관리한다.
class GameSession
{
public:
    void reset();
    void update(double deltaTime, BoardState& state, bool& hasSelection, vector<TileAnimation>& animations);
    void registerClear(int clearedCount, int spawnCount);
    void breakCombo();

    int getScore() const;
    int getCombo() const;
    int getTimeRemainingSeconds() const;
    float getGameOverPulse() const;
    bool isGameOver() const;

private:
    int _score = 0;
    int _combo = 0;
    float _timeRemaining = PuzzleRoundTimeSeconds;
    float _gameOverPulse = 0.0f;
};

#endif // !GAMESESSION_H_
