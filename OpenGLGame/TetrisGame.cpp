#include "pch.h"
#include "TetrisGame.h"

namespace
{
    using ShapeGrid = std::array<std::array<int, 4>, 4>;

    // 7개의 테트로미노를 4x4 격자 기준으로 저장한다.
    // 회전 결과를 미리 전부 펼쳐두면 런타임에 회전 좌표를 계산할 필요가 없어서
    // 충돌 판정, 고스트 피스 계산, 스폰 검사까지 모두 같은 데이터로 처리할 수 있다.
    constexpr std::array<std::array<ShapeGrid, 4>, 7> kTetrominoes = { {
        { {
            { { {0, 0, 0, 0}, {1, 1, 1, 1}, {0, 0, 0, 0}, {0, 0, 0, 0} } },
            { { {0, 0, 1, 0}, {0, 0, 1, 0}, {0, 0, 1, 0}, {0, 0, 1, 0} } },
            { { {0, 0, 0, 0}, {0, 0, 0, 0}, {1, 1, 1, 1}, {0, 0, 0, 0} } },
            { { {0, 1, 0, 0}, {0, 1, 0, 0}, {0, 1, 0, 0}, {0, 1, 0, 0} } }
        } },
        { {
            { { {0, 1, 1, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} } },
            { { {0, 1, 1, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} } },
            { { {0, 1, 1, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} } },
            { { {0, 1, 1, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} } }
        } },
        { {
            { { {0, 1, 0, 0}, {1, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} } },
            { { {0, 1, 0, 0}, {0, 1, 1, 0}, {0, 1, 0, 0}, {0, 0, 0, 0} } },
            { { {0, 0, 0, 0}, {1, 1, 1, 0}, {0, 1, 0, 0}, {0, 0, 0, 0} } },
            { { {0, 1, 0, 0}, {1, 1, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0} } }
        } },
        { {
            { { {0, 1, 1, 0}, {1, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} } },
            { { {0, 1, 0, 0}, {0, 1, 1, 0}, {0, 0, 1, 0}, {0, 0, 0, 0} } },
            { { {0, 0, 0, 0}, {0, 1, 1, 0}, {1, 1, 0, 0}, {0, 0, 0, 0} } },
            { { {1, 0, 0, 0}, {1, 1, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0} } }
        } },
        { {
            { { {1, 1, 0, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} } },
            { { {0, 0, 1, 0}, {0, 1, 1, 0}, {0, 1, 0, 0}, {0, 0, 0, 0} } },
            { { {0, 0, 0, 0}, {1, 1, 0, 0}, {0, 1, 1, 0}, {0, 0, 0, 0} } },
            { { {0, 1, 0, 0}, {1, 1, 0, 0}, {1, 0, 0, 0}, {0, 0, 0, 0} } }
        } },
        { {
            { { {1, 0, 0, 0}, {1, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} } },
            { { {0, 1, 1, 0}, {0, 1, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0} } },
            { { {0, 0, 0, 0}, {1, 1, 1, 0}, {0, 0, 1, 0}, {0, 0, 0, 0} } },
            { { {0, 1, 0, 0}, {0, 1, 0, 0}, {1, 1, 0, 0}, {0, 0, 0, 0} } }
        } },
        { {
            { { {0, 0, 1, 0}, {1, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} } },
            { { {0, 1, 0, 0}, {0, 1, 0, 0}, {0, 1, 1, 0}, {0, 0, 0, 0} } },
            { { {0, 0, 0, 0}, {1, 1, 1, 0}, {1, 0, 0, 0}, {0, 0, 0, 0} } },
            { { {1, 1, 0, 0}, {0, 1, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0} } }
        } }
    } };

    constexpr std::array<int, 5> kLineScores = { 0, 100, 300, 500, 800 };
}

TetrisGame::TetrisGame()
{
    reset();
}

void TetrisGame::reset()
{
    // resetEventId_는 렌더러/오디오가 "새 라운드가 시작됐다"는 사실을 감지하는 신호다.
    // 단순히 상태만 0으로 만드는 대신 이벤트 카운터를 올려서 외부 시스템도 함께 초기화한다.
    ++resetEventId_;
    resetBoard();
    resetStats();
    bagCursor_ = static_cast<int>(pieceBag_.size());
    nextPiece_ = createSpawnPiece(drawFromBag());
    spawnNextPiece();
}

void TetrisGame::update(double deltaTime)
{
    if (gameOver_ || paused_)
    {
        return;
    }

    if (lineClearAnimationTimer_ > 0.0)
    {
        // 줄 삭제는 곧바로 보드를 당기지 않고 짧게 멈춘다.
        // 이 시간 동안 렌더러는 깜빡임 연출을 보여주고, 오디오는 "라인 클리어" 이벤트를 재생한다.
        lineClearAnimationTimer_ = std::max(0.0, lineClearAnimationTimer_ - deltaTime);
        if (lineClearAnimationTimer_ == 0.0)
        {
            finalizeLineClear();
        }
        return;
    }

    // 평상시에는 누적 시간을 중력 타이머처럼 사용해서 일정 간격마다 한 칸씩 떨어뜨린다.
    fallAccumulator_ += deltaTime;
    if (fallAccumulator_ < getFallInterval())
    {
        return;
    }

    fallAccumulator_ = 0.0;
    if (!movePiece(0, 1))
    {
        lockCurrentPiece();
    }
}

bool TetrisGame::movePiece(int deltaX, int deltaY)
{
    if (gameOver_ || paused_ || isAnimatingLineClear())
    {
        return false;
    }

    ActivePiece movedPiece = currentPiece_;
    movedPiece.x += deltaX;
    movedPiece.y += deltaY;

    // 실제 piece를 바로 바꾸지 않고 후보 위치를 만든 뒤 판정하는 이유는,
    // 이동/회전/고스트 계산 모두가 같은 "fit 검사" 패턴을 공유하기 위해서다.
    if (!doesPieceFit(movedPiece))
    {
        return false;
    }

    currentPiece_ = movedPiece;
    return true;
}

bool TetrisGame::rotatePieceClockwise()
{
    if (gameOver_ || paused_ || isAnimatingLineClear())
    {
        return false;
    }

    ActivePiece rotatedPiece = currentPiece_;
    rotatedPiece.rotation = (rotatedPiece.rotation + 1) % 4;

    // 완전한 SRS 구현은 아니지만, 벽 바로 옆에서도 회전이 답답하지 않도록
    // 좌우로 몇 칸 보정해 보는 간단한 wall kick을 적용한다.
    static constexpr std::array<int, 5> kicks = { 0, -1, 1, -2, 2 };
    for (const int kick : kicks)
    {
        ActivePiece candidate = rotatedPiece;
        candidate.x += kick;
        if (doesPieceFit(candidate))
        {
            currentPiece_ = candidate;
            return true;
        }
    }

    return false;
}

bool TetrisGame::rotatePieceCounterClockwise()
{
    if (gameOver_ || paused_ || isAnimatingLineClear())
    {
        return false;
    }

    ActivePiece rotatedPiece = currentPiece_;
    rotatedPiece.rotation = (rotatedPiece.rotation + 3) % 4;

    static constexpr std::array<int, 5> kicks = { 0, -1, 1, -2, 2 };
    for (const int kick : kicks)
    {
        ActivePiece candidate = rotatedPiece;
        candidate.x += kick;
        if (doesPieceFit(candidate))
        {
            currentPiece_ = candidate;
            return true;
        }
    }

    return false;
}

bool TetrisGame::softDrop()
{
    if (gameOver_ || paused_ || isAnimatingLineClear())
    {
        return false;
    }

    if (movePiece(0, 1))
    {
        // 소프트 드롭은 사용자가 능동적으로 빠르게 내린 보상으로
        // 한 칸당 소량의 점수를 추가한다.
        ++score_;
        return true;
    }

    lockCurrentPiece();
    fallAccumulator_ = 0.0;
    return false;
}

void TetrisGame::hardDrop()
{
    if (gameOver_ || paused_ || isAnimatingLineClear())
    {
        return;
    }

    int droppedRows = 0;
    // 더 이상 내려갈 수 없을 때까지 한 번에 이동시키고,
    // 이동 거리만큼 추가 점수를 계산한다.
    while (movePiece(0, 1))
    {
        ++droppedRows;
    }

    score_ += droppedRows * 2;
    ++hardDropEventId_;
    lockCurrentPiece();
    fallAccumulator_ = 0.0;
}

bool TetrisGame::holdCurrentPiece()
{
    if (gameOver_ || paused_ || isAnimatingLineClear() || !canHoldCurrentPiece_)
    {
        return false;
    }

    const int previousHeldShape = heldShape_;
    heldShape_ = currentPiece_.shape;

    // 첫 홀드는 현재 피스를 저장하고 next 큐에서 새 피스를 가져온다.
    // 두 번째 홀드부터는 저장된 피스와 현재 피스를 교환한다.
    // 한 턴에 한 번만 허용하는 이유는 홀드 무한 반복으로 난이도가 무너지는 것을 막기 위해서다.
    if (previousHeldShape < 0)
    {
        spawnNextPiece();
    }
    else
    {
        currentPiece_ = createSpawnPiece(previousHeldShape);
        if (!doesPieceFit(currentPiece_))
        {
            setGameOver();
        }
    }

    canHoldCurrentPiece_ = false;
    fallAccumulator_ = 0.0;
    ++holdEventId_;
    return true;
}

void TetrisGame::togglePause()
{
    if (gameOver_)
    {
        return;
    }

    paused_ = !paused_;
}

bool TetrisGame::isGameOver() const { return gameOver_; }
bool TetrisGame::isPaused() const { return paused_; }
bool TetrisGame::isAnimatingLineClear() const { return lineClearAnimationTimer_ > 0.0; }
int TetrisGame::getScore() const { return score_; }
int TetrisGame::getLinesCleared() const { return linesCleared_; }
int TetrisGame::getLevel() const { return level_; }
int TetrisGame::getComboCount() const { return std::max(0, comboCount_); }
int TetrisGame::getHeldShape() const { return heldShape_; }
bool TetrisGame::hasHeldPiece() const { return heldShape_ >= 0; }
double TetrisGame::getFallInterval() const
{
    // 레벨별 낙하 속도는 수식 하나로 만들기보다 사람이 체감하기 좋은 값으로 직접 조정했다.
    // 초반은 학습 가능한 속도로 유지하고, 후반은 테이블 이후에도 조금씩 더 빨라지게 만든다.
    static constexpr std::array<double, 15> kFallCurve = {
        0.72, 0.64, 0.57, 0.50, 0.44,
        0.39, 0.34, 0.30, 0.26, 0.23,
        0.20, 0.18, 0.16, 0.145, 0.13
    };

    const int index = std::clamp(level_ - 1, 0, static_cast<int>(kFallCurve.size()) - 1);
    const double baseInterval = kFallCurve[index];
    if (level_ <= static_cast<int>(kFallCurve.size()))
    {
        return baseInterval;
    }

    const double extraDrop = (level_ - static_cast<int>(kFallCurve.size())) * 0.006;
    return std::max(0.07, baseInterval - extraDrop);
}
double TetrisGame::getLineClearAnimationProgress() const
{
    if (!isAnimatingLineClear())
    {
        return 0.0;
    }

    return 1.0 - (lineClearAnimationTimer_ / LineClearAnimationDuration);
}
uint64_t TetrisGame::getLineClearEventId() const { return lineClearEventId_; }
uint64_t TetrisGame::getHardDropEventId() const { return hardDropEventId_; }
uint64_t TetrisGame::getHoldEventId() const { return holdEventId_; }
uint64_t TetrisGame::getGameOverEventId() const { return gameOverEventId_; }
uint64_t TetrisGame::getLevelUpEventId() const { return levelUpEventId_; }
uint64_t TetrisGame::getResetEventId() const { return resetEventId_; }
int TetrisGame::getLastClearedRowCount() const { return lastClearedRowCount_; }
const std::array<int, 4>& TetrisGame::getLastClearedRows() const { return lastClearedRows_; }
const std::array<bool, TetrisGame::BoardHeight>& TetrisGame::getClearingRows() const { return clearingRows_; }
const TetrisGame::ActivePiece& TetrisGame::getCurrentPiece() const { return currentPiece_; }
const TetrisGame::ActivePiece& TetrisGame::getNextPiece() const { return nextPiece_; }
const TetrisGame::ActivePiece& TetrisGame::getLastLockedPiece() const { return lastLockedPiece_; }
TetrisGame::ActivePiece TetrisGame::getGhostPiece() const
{
    ActivePiece ghost = currentPiece_;
    // 고스트 피스는 현재 피스를 복사한 뒤, 충돌 직전까지 아래로 내린 결과다.
    // 렌더러는 이 값을 반투명하게 그려 "지금 드롭하면 어디에 닿는지"를 보여준다.
    while (doesPieceFit({ ghost.shape, ghost.rotation, ghost.x, ghost.y + 1 }))
    {
        ++ghost.y;
    }
    return ghost;
}
const std::array<std::array<int, TetrisGame::BoardWidth>, TetrisGame::BoardHeight>& TetrisGame::getBoard() const { return board_; }

bool TetrisGame::isFilledCell(int shape, int rotation, int row, int column)
{
    return kTetrominoes[shape][rotation][row][column] != 0;
}

void TetrisGame::resetBoard()
{
    for (auto& row : board_)
    {
        row.fill(0);
    }
}

void TetrisGame::resetStats()
{
    fallAccumulator_ = 0.0;
    lineClearAnimationTimer_ = 0.0;
    gameOver_ = false;
    paused_ = false;
    canHoldCurrentPiece_ = true;
    score_ = 0;
    linesCleared_ = 0;
    level_ = 1;
    heldShape_ = -1;
    comboCount_ = -1;
    pendingClearedLines_ = 0;
    clearingRows_.fill(false);
    lastClearedRows_.fill(-1);
    lastClearedRowCount_ = 0;
    lineClearEventId_ = 0;
    hardDropEventId_ = 0;
    holdEventId_ = 0;
    gameOverEventId_ = 0;
    levelUpEventId_ = 0;
    lastLockedPiece_ = {};
}

void TetrisGame::refillBag()
{
    for (int index = 0; index < static_cast<int>(pieceBag_.size()); ++index)
    {
        pieceBag_[index] = index;
    }

    static std::random_device rd;
    static std::mt19937 generator(rd());
    std::shuffle(pieceBag_.begin(), pieceBag_.end(), generator);
    bagCursor_ = 0;
}

int TetrisGame::drawFromBag()
{
    if (bagCursor_ >= static_cast<int>(pieceBag_.size()))
    {
        refillBag();
    }

    return pieceBag_[bagCursor_++];
}

TetrisGame::ActivePiece TetrisGame::createSpawnPiece(int shape) const
{
    ActivePiece piece;
    piece.shape = shape;
    piece.rotation = 0;
    piece.x = 3;
    piece.y = 0;
    return piece;
}

void TetrisGame::spawnNextPiece()
{
    currentPiece_ = nextPiece_;
    nextPiece_ = createSpawnPiece(drawFromBag());
    nextPiece_.x = 0;
    nextPiece_.y = 0;
    canHoldCurrentPiece_ = true;

    if (!doesPieceFit(currentPiece_))
    {
        setGameOver();
    }
}

bool TetrisGame::doesPieceFit(const ActivePiece& piece) const
{
    // 이 함수는 테트리스 규칙의 핵심 판정이다.
    // 이동, 회전, 스폰 가능 여부, 홀드 교체 가능 여부, 고스트 피스 계산이 모두 여기로 모인다.
    // 그래서 충돌 관련 버그를 잡을 때 가장 먼저 봐야 하는 지점이기도 하다.
    for (int row = 0; row < 4; ++row)
    {
        for (int column = 0; column < 4; ++column)
        {
            if (!isFilledCell(piece.shape, piece.rotation, row, column))
            {
                continue;
            }

            const int boardX = piece.x + column;
            const int boardY = piece.y + row;

            if (boardX < 0 || boardX >= BoardWidth || boardY < 0 || boardY >= BoardHeight)
            {
                return false;
            }

            if (board_[boardY][boardX] != 0)
            {
                return false;
            }
        }
    }

    return true;
}

void TetrisGame::lockCurrentPiece()
{
    lastLockedPiece_ = currentPiece_;

    // 현재 피스를 보드에 굳힌 뒤, 라인이 지워지는지 먼저 확인한다.
    // 줄이 있으면 즉시 다음 피스를 뽑지 않고 애니메이션 상태로 들어가고,
    // 줄이 없으면 바로 다음 피스를 스폰한다.
    for (int row = 0; row < 4; ++row)
    {
        for (int column = 0; column < 4; ++column)
        {
            if (!isFilledCell(currentPiece_.shape, currentPiece_.rotation, row, column))
            {
                continue;
            }

            const int boardX = currentPiece_.x + column;
            const int boardY = currentPiece_.y + row;
            if (boardX >= 0 && boardX < BoardWidth && boardY >= 0 && boardY < BoardHeight)
            {
                board_[boardY][boardX] = currentPiece_.shape + 1;
            }
        }
    }

    const int clearedLines = beginLineClear();
    if (clearedLines > 0)
    {
        lineClearAnimationTimer_ = LineClearAnimationDuration;
        pendingClearedLines_ = clearedLines;
        return;
    }

    updateScore(0);
    spawnNextPiece();
}

int TetrisGame::beginLineClear()
{
    // 삭제 대상 줄을 먼저 "표시만" 해두고 실제 보드 압축은 나중에 한다.
    // 이렇게 분리하면 렌더러는 깜빡임을 그릴 수 있고, 파티클/오디오도
    // 정확히 어떤 줄이 지워졌는지를 같은 프레임에서 참조할 수 있다.
    clearingRows_.fill(false);
    lastClearedRows_.fill(-1);
    lastClearedRowCount_ = 0;

    for (int row = BoardHeight - 1; row >= 0; --row)
    {
        const bool full = std::all_of(board_[row].begin(), board_[row].end(), [](int cell) { return cell != 0; });
        if (!full)
        {
            continue;
        }

        clearingRows_[row] = true;
        if (lastClearedRowCount_ < static_cast<int>(lastClearedRows_.size()))
        {
            lastClearedRows_[lastClearedRowCount_++] = row;
        }
    }

    return lastClearedRowCount_;
}

void TetrisGame::finalizeLineClear()
{
    if (pendingClearedLines_ <= 0)
    {
        return;
    }

    std::array<std::array<int, BoardWidth>, BoardHeight> collapsedBoard{};
    int writeRow = BoardHeight - 1;
    // 아래에서 위로 다시 쌓아 올리면서 삭제 대상 줄만 건너뛴다.
    // 결과적으로 "지워지지 않은 줄들이 아래로 떨어지는" 전형적인 테트리스 보드 압축이 된다.
    for (int row = BoardHeight - 1; row >= 0; --row)
    {
        if (clearingRows_[row])
        {
            continue;
        }

        collapsedBoard[writeRow--] = board_[row];
    }

    while (writeRow >= 0)
    {
        collapsedBoard[writeRow--].fill(0);
    }

    board_ = collapsedBoard;
    clearingRows_.fill(false);
    lineClearAnimationTimer_ = 0.0;

    updateScore(pendingClearedLines_);
    // 이 이벤트 카운터는 실제 줄 삭제가 끝난 시점에만 증가한다.
    // 그래서 사운드/파티클이 "표시만 된 순간"이 아니라 "정말 클리어된 순간"에 반응한다.
    ++lineClearEventId_;
    pendingClearedLines_ = 0;
    spawnNextPiece();
}

void TetrisGame::updateScore(int clearedLines)
{
    if (clearedLines <= 0)
    {
        // 줄을 지우지 못한 턴이 나오면 콤보는 끊긴다.
        comboCount_ = -1;
        return;
    }

    ++comboCount_;
    linesCleared_ += clearedLines;
    // 레벨 증가는 점수가 아니라 누적 삭제 줄 수를 기준으로 계산한다.
    // 그래야 콤보 보너스가 많아져도 낙하 속도는 항상 예측 가능한 패턴을 유지한다.
    const int previousLevel = level_;
    level_ = 1 + (linesCleared_ / 8);
    if (level_ > previousLevel)
    {
        ++levelUpEventId_;
    }
    score_ += kLineScores[std::min(clearedLines, static_cast<int>(kLineScores.size()) - 1)] * level_;
    score_ += comboCount_ * 50 * level_;
}

void TetrisGame::setGameOver()
{
    if (!gameOver_)
    {
        // 게임 오버도 이벤트 카운터로 외부에 알린다.
        // 오디오/오버레이가 "상태가 이미 gameOver였다"는 이유로 신호를 놓치지 않게 하기 위함이다.
        gameOver_ = true;
        ++gameOverEventId_;
    }
}
