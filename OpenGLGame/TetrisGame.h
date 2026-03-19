#pragma once

#ifndef TETRISGAME_H_
#define TETRISGAME_H_

#include "pch.h"

class TetrisGame
{
public:
    struct ActivePiece
    {
        int shape = 0;
        int rotation = 0;
        int x = 3;
        int y = 0;
    };

    static constexpr int BoardWidth = 10;
    static constexpr int BoardHeight = 20;
    static constexpr double LineClearAnimationDuration = 0.18;

public:
    TetrisGame();

    void reset();
    void update(double deltaTime);

    bool movePiece(int deltaX, int deltaY);
    bool rotatePieceClockwise();
    bool rotatePieceCounterClockwise();
    bool softDrop();
    void hardDrop();
    bool holdCurrentPiece();
    void togglePause();

    bool isGameOver() const;
    bool isPaused() const;
    bool isAnimatingLineClear() const;
    int getScore() const;
    int getLinesCleared() const;
    int getLevel() const;
    int getComboCount() const;
    int getHeldShape() const;
    bool hasHeldPiece() const;
    double getFallInterval() const;
    double getLineClearAnimationProgress() const;
    uint64_t getLineClearEventId() const;
    uint64_t getHardDropEventId() const;
    uint64_t getHoldEventId() const;
    uint64_t getGameOverEventId() const;
    uint64_t getLevelUpEventId() const;
    uint64_t getResetEventId() const;
    int getLastClearedRowCount() const;
    const std::array<int, 4>& getLastClearedRows() const;
    const std::array<bool, BoardHeight>& getClearingRows() const;

    const ActivePiece& getCurrentPiece() const;
    const ActivePiece& getNextPiece() const;
    const ActivePiece& getLastLockedPiece() const;
    ActivePiece getGhostPiece() const;
    const std::array<std::array<int, BoardWidth>, BoardHeight>& getBoard() const;

    static bool isFilledCell(int shape, int rotation, int row, int column);

private:
    std::array<std::array<int, BoardWidth>, BoardHeight> board_{};
    std::array<int, 7> pieceBag_{};
    int bagCursor_ = 7;

    ActivePiece currentPiece_{};
    ActivePiece nextPiece_{};
    ActivePiece lastLockedPiece_{};
    int heldShape_ = -1;

    double fallAccumulator_ = 0.0;
    double lineClearAnimationTimer_ = 0.0;
    bool gameOver_ = false;
    bool paused_ = false;
    bool canHoldCurrentPiece_ = true;
    int score_ = 0;
    int linesCleared_ = 0;
    int level_ = 1;
    int comboCount_ = -1;
    int pendingClearedLines_ = 0;
    std::array<bool, BoardHeight> clearingRows_{};
    std::array<int, 4> lastClearedRows_{};
    int lastClearedRowCount_ = 0;
    uint64_t lineClearEventId_ = 0;
    uint64_t hardDropEventId_ = 0;
    uint64_t holdEventId_ = 0;
    uint64_t gameOverEventId_ = 0;
    uint64_t levelUpEventId_ = 0;
    uint64_t resetEventId_ = 0;

private:
    void resetBoard();
    void resetStats();
    void refillBag();
    int drawFromBag();
    ActivePiece createSpawnPiece(int shape) const;
    void spawnNextPiece();
    bool doesPieceFit(const ActivePiece& piece) const;
    void lockCurrentPiece();
    int beginLineClear();
    void finalizeLineClear();
    void updateScore(int clearedLines);
    void setGameOver();
};

#endif // !TETRISGAME_H_
