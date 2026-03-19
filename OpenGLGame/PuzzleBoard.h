#pragma once

#ifndef PUZZLEBOARD_H_
#define PUZZLEBOARD_H_

#include "BoardAnimator.h"
#include "BoardStateMachine.h"
#include "CascadeResolver.h"
#include "GameSession.h"
#include "MatchFactory.h"
#include "PuzzleRenderer.h"
#include "PuzzleInputController.h"
#include "ShuffleResolver.h"
#include "SpawnResolver.h"
#include "SpecialEffectSystem.h"
#include "SpecialComboResolver.h"
#include "SwapResolver.h"

// PuzzleBoard는 실제 게임 턴을 조율하는 상위 오케스트레이터다.
// 입력을 받아 어떤 리졸버를 호출할지 결정하고,
// 상태 전이와 세션/애니메이션/렌더러를 서로 연결하는 역할을 맡는다.
class PuzzleBoard
{
public:
    PuzzleBoard();

    void initialize();
    void resize(int screenWidth, int screenHeight);
    void update(double deltaTime);
    void render(const mat4& orthoProjection);
    void handleClick(const ivec2& cursorPosition);
    void reset();

    int getScore() const;
    int getCombo() const;
    int getTimeRemainingSeconds() const;
    bool isGameOver() const;

private:
    // 한 번의 턴에서 반복적으로 쓰이는 연출 시간 상수들이다.
    static constexpr float SwapDuration = 0.12f;
    static constexpr float ClearDuration = 0.16f;
    static constexpr float FallDurationPerCell = 0.05f;
    static constexpr float MinimumFallDuration = 0.10f;
    static constexpr float EffectDuration = 0.22f;

    // 실제 보드 데이터와 보드 주변 시스템들이다.
    PuzzleGrid _tiles{};
    PuzzleRenderer _renderer;
    BoardAnimator _animator;
    GameSession _session;
    mt19937 _rng;

    // 현재 턴 진행 상태를 추적하기 위한 필드들이다.
    bool _initialized = false;
    bool _hasSelection = false;
    Cell _selectedCell{};
    Cell _swapSource{};
    Cell _swapTarget{};
    MatchResolution _currentResolution;
    MatchResolution _pendingSwapResolution;
    BoardState _state = BoardState::WaitingForInput;

private:
    // 시작 직후 자동 매치가 없는 보드를 만든다.
    void buildBoardWithoutStartingMatches();
    // 더 이상 둘 수 있는 수가 없으면 셔플해서 플레이 가능한 보드로 보정한다.
    void ensurePlayableBoard();
    // 선택된 칸과 대상 칸의 스왑을 시도하고, 이후 상태 전이를 예약한다.
    bool trySwapSelection(const Cell& targetCell);
    // 특정 칸에서 특수/일반 제거 이펙트를 추가한다.
    void addBurst(const Cell& cell, const Tile& tile, float scale = 1.15f);
    // 두 타일이 서로 자리를 바꾸는 애니메이션을 시작한다.
    void beginSwapAnimation(const Cell& first, const Cell& second, BoardState nextState);
    // 이번 매치 결과를 화면에 번쩍임으로 보여주는 단계다.
    void beginClearAnimation(const MatchResolution& resolution);
    // 클리어 애니메이션이 끝난 뒤 실제 타일 제거와 점수 반영을 수행한다.
    void resolveClearedTiles();
    // 비어 있는 칸을 메우는 낙하/리필 애니메이션을 시작한다.
    void beginFallAnimation();
    // 현재 상태의 애니메이션이 끝났을 때 다음 파이프라인 단계로 넘긴다.
    void finishAnimationStep();
    // 진행 중인 타일 애니메이션을 갱신하고 종료 시 다음 단계를 호출한다.
    void updateAnimations(double deltaTime);
    // 화면 효과 버스트만 따로 갱신한다.
    void updateEffects(double deltaTime);
};

#endif // !PUZZLEBOARD_H_
