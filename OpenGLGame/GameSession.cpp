#include "pch.h"
#include "GameSession.h"

void GameSession::reset()
{
    _score = 0;
    _combo = 0;
    _timeRemaining = PuzzleRoundTimeSeconds;
    _gameOverPulse = 0.0f;
}

void GameSession::update(double deltaTime, BoardState& state, bool& hasSelection, vector<TileAnimation>& animations)
{
    // 게임오버 전까지는 시간이 계속 흐르고,
    // 시간이 다 되면 보드를 TimeUp 상태로 전환해 입력과 애니메이션을 멈춘다.
    if (state != BoardState::TimeUp)
    {
        _timeRemaining = std::max(0.0f, _timeRemaining - static_cast<float>(deltaTime));
        if (_timeRemaining <= 0.0f)
        {
            _timeRemaining = 0.0f;
            state = BoardState::TimeUp;
            hasSelection = false;
            animations.clear();
        }
    }

    _gameOverPulse += static_cast<float>(deltaTime);
}

void GameSession::registerClear(int clearedCount, int spawnCount)
{
    // 연쇄가 이어질수록 콤보 배수를 올리고,
    // 특수 블록 생성은 일반 제거보다 추가 보너스를 준다.
    _combo += 1;
    const int comboMultiplier = std::max(1, _combo);
    _score += clearedCount * 10 * comboMultiplier;
    _score += spawnCount * 40;
    _timeRemaining = std::min(PuzzleRoundTimeSeconds, _timeRemaining + (0.6f * static_cast<float>(comboMultiplier)));
}

void GameSession::breakCombo()
{
    // 연쇄가 더 이상 이어지지 않으면 다음 클리어부터 다시 1콤보로 시작한다.
    _combo = 0;
}

int GameSession::getScore() const
{
    return _score;
}

int GameSession::getCombo() const
{
    return _combo;
}

int GameSession::getTimeRemainingSeconds() const
{
    return static_cast<int>(ceilf(_timeRemaining));
}

float GameSession::getGameOverPulse() const
{
    return _gameOverPulse;
}

bool GameSession::isGameOver() const
{
    return _timeRemaining <= 0.0f;
}
