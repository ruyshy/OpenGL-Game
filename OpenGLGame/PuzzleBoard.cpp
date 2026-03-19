#include "pch.h"
#include "PuzzleBoard.h"
#include "PuzzleRuleEngine.h"

PuzzleBoard::PuzzleBoard()
    : _rng(random_device{}())
{
}

void PuzzleBoard::initialize()
{
    // 렌더러 초기화는 한 번만 수행하고, 실제 게임 데이터는 reset에서 다시 만든다.
    if (_initialized)
    {
        return;
    }

    _renderer.initialize();
    reset();
    _initialized = true;
}

void PuzzleBoard::resize(int screenWidth, int screenHeight)
{
    _renderer.resize(screenWidth, screenHeight);
}

void PuzzleBoard::update(double deltaTime)
{
    // 세션은 시간 제한과 게임오버 전환을 담당하고,
    // 보드는 그 위에 애니메이션과 이펙트를 별도로 갱신한다.
    _session.update(deltaTime, _state, _hasSelection, _animator.animations());
    updateAnimations(deltaTime);
    updateEffects(deltaTime);
}

void PuzzleBoard::render(const mat4& orthoProjection)
{
    _renderer.render(
        orthoProjection,
        _tiles,
        _animator.animations(),
        _animator.effects(),
        _hasSelection,
        _selectedCell,
        _state,
        _session.getGameOverPulse());
}

void PuzzleBoard::handleClick(const ivec2& cursorPosition)
{
    // 입력은 사용자가 실제로 조작 가능한 대기 상태에서만 받는다.
    if (_state != BoardState::WaitingForInput)
    {
        return;
    }

    const Cell clickedCell = _renderer.toCell(cursorPosition);
    if (!PuzzleRuleEngine::isInsideBoard(clickedCell))
    {
        _hasSelection = false;
        return;
    }

    // 클릭 해석 자체는 입력 컨트롤러에 맡기고,
    // 보드는 그 결과에 따라 선택/해제/스왑만 오케스트레이션한다.
    const InputAction action = PuzzleInputController::processClick(_hasSelection, _selectedCell, clickedCell);
    if (action.type == InputActionType::Select)
    {
        _selectedCell = action.target;
        _hasSelection = true;
        return;
    }
    if (action.type == InputActionType::Deselect)
    {
        _hasSelection = false;
        return;
    }
    if (action.type == InputActionType::Swap)
    {
        trySwapSelection(action.target);
    }
}

void PuzzleBoard::reset()
{
    // 리셋은 "세션/애니메이션/현재 턴 정보"를 모두 초기화한 뒤
    // 새 보드를 만들어서 완전히 처음 상태로 되돌린다.
    _hasSelection = false;
    _animator.clearAll();
    _currentResolution = {};
    _pendingSwapResolution = {};
    _state = BoardStateMachine::onWaitingForInput();
    _session.reset();
    buildBoardWithoutStartingMatches();
}

int PuzzleBoard::getScore() const
{
    return _session.getScore();
}

int PuzzleBoard::getCombo() const
{
    return _session.getCombo();
}

int PuzzleBoard::getTimeRemainingSeconds() const
{
    return _session.getTimeRemainingSeconds();
}

bool PuzzleBoard::isGameOver() const
{
    return _session.isGameOver();
}

void PuzzleBoard::buildBoardWithoutStartingMatches()
{
    // 시작 보드는 자동 매치가 없어야 하고, 동시에 둘 수 있는 수도 있어야 한다.
    SpawnResolver::buildInitialBoard(_tiles, _rng);
    ensurePlayableBoard();
}

void PuzzleBoard::ensurePlayableBoard()
{
    // 무작정 새 보드를 다시 만드는 대신, 현재 보드의 타일 풀을 섞어서 재활용한다.
    if (!ShuffleResolver::hasAvailableMove(_tiles))
    {
        ShuffleResolver::shuffleUntilPlayable(_tiles, _rng);
    }
}

bool PuzzleBoard::trySwapSelection(const Cell& targetCell)
{
    const Cell originalSelection = _selectedCell;
    // 특수 블록 조합은 일반 3매치 검사보다 우선한다.
    _pendingSwapResolution = MatchFactory::createSpecialSwapMatch(_tiles, originalSelection, targetCell);
    const bool validSwap = !_pendingSwapResolution.cells.empty() || SwapResolver::createsMatch(_tiles, originalSelection, targetCell);

    _swapSource = originalSelection;
    _swapTarget = targetCell;
    _hasSelection = false;
    beginSwapAnimation(
        originalSelection,
        targetCell,
        BoardStateMachine::stateForSwapResult(validSwap));

    if (validSwap)
    {
        // 유효한 스왑일 때만 실제 보드 데이터를 먼저 바꾼다.
        // 이후 애니메이션 종료 시점에 매치 계산이 자연스럽게 이어진다.
        SwapResolver::applySwap(_tiles, originalSelection, targetCell);
    }
    else
    {
        // 실패한 스왑은 원래 선택을 유지하지 않고, 방금 클릭한 칸을 새 선택 상태로 둔다.
        _selectedCell = targetCell;
        _hasSelection = true;
    }

    return validSwap;
}

void PuzzleBoard::addBurst(const Cell& cell, const Tile& tile, float scale)
{
    // 현재는 타일 중심 기준의 단순 사각형 버스트를 사용한다.
    const vec2 center = _renderer.getCellPosition(cell) + vec2(28.0f, 28.0f);
    EffectBurst effect = SpecialEffectSystem::makeBurstForTile(center, tile, 56.0f, EffectDuration);
    effect.size = vec2(56.0f * scale);
    _animator.addEffect(effect);
}

void PuzzleBoard::beginSwapAnimation(const Cell& first, const Cell& second, BoardState nextState)
{
    _animator.beginSwap(_tiles[first.row][first.column], _tiles[second.row][second.column], _renderer.getCellPosition(first), _renderer.getCellPosition(second), SwapDuration);
    _state = nextState;
}

void PuzzleBoard::beginClearAnimation(const MatchResolution& resolution)
{
    _animator.animations().clear();
    _currentResolution = resolution;

    for (const Cell& cell : resolution.cells)
    {
        // 특수 블록 생성 위치는 사라지는 대신 "남아야" 하므로
        // 클리어 플래시 대상에서 제외한다.
        bool isSpawnAnchor = false;
        for (const SpecialSpawn& spawn : resolution.spawns)
        {
            if (spawn.cell == cell)
            {
                isSpawnAnchor = true;
                break;
            }
        }

        if (!isSpawnAnchor)
        {
            _animator.addFlash(_tiles[cell.row][cell.column], _renderer.getCellPosition(cell), ClearDuration);
        }
    }

    _state = BoardStateMachine::onClearStarted();
}

void PuzzleBoard::resolveClearedTiles()
{
    // 먼저 일반 제거 결과를 특수 블록 연쇄까지 확장해
    // 이번 단계에서 실제로 비워질 모든 칸을 얻는다.
    const ExpandedClearResult expanded = CascadeResolver::expandClears(_tiles, _currentResolution);
    set<pair<int, int>> cellsToClear;

    for (const Cell& cell : expanded.clearedCells)
    {
        cellsToClear.insert({ cell.column, cell.row });
    }

    for (const SpecialSpawn& spawn : _currentResolution.spawns)
    {
        // 새 특수 블록이 생길 자리까지 비워버리면 안 되므로
        // 최종 제거 목록에서 생성 앵커는 제외한다.
        cellsToClear.erase({ spawn.cell.column, spawn.cell.row });
    }

    for (const SpecialSpawn& activated : expanded.activatedSpecials)
    {
        // 발동한 특수 블록은 일반 블록보다 조금 더 큰 버스트로 강조한다.
        addBurst(
            activated.cell,
            activated.tile,
            activated.tile.special == SpecialType::ColorBomb ? 3.0f :
            (activated.tile.special == SpecialType::Bomb ? 2.4f : 1.8f));
    }

    for (const auto& entry : cellsToClear)
    {
        const Cell cell{ entry.first, entry.second };
        if (!PuzzleRuleEngine::isEmpty(_tiles[cell.row][cell.column]))
        {
            addBurst(cell, _tiles[cell.row][cell.column], 1.15f);
        }
        _tiles[cell.row][cell.column] = {};
    }

    for (const SpecialSpawn& spawn : _currentResolution.spawns)
    {
        // 제거가 끝난 뒤 마지막에 생성 블록을 다시 배치해야
        // 기존 타일 제거와 충돌하지 않는다.
        _tiles[spawn.cell.row][spawn.cell.column] = spawn.tile;
        addBurst(spawn.cell, spawn.tile, 1.35f);
    }

    _session.registerClear(static_cast<int>(cellsToClear.size()), static_cast<int>(_currentResolution.spawns.size()));
    _currentResolution = {};
}

void PuzzleBoard::beginFallAnimation()
{
    _animator.animations().clear();

    // buildFall은 다음 보드 상태와 그 상태로 이동하는 낙하 애니메이션 목록을 함께 돌려준다.
    const CascadeFallResult fall = CascadeResolver::buildFall(_tiles, _rng, _renderer, MinimumFallDuration, FallDurationPerCell);
    _tiles = fall.nextTiles;
    for (const TileAnimation& animation : fall.falls)
    {
        _animator.addFall(animation.tile, animation.start, animation.end, animation.duration);
    }

    if (_animator.animations().empty())
    {
        // 낙하가 하나도 없다는 것은 곧바로 다음 매치 판정으로 넘어가도 된다는 뜻이다.
        const MatchResolution resolution = MatchFactory::createBoardMatch(_tiles, _swapSource, _swapTarget);
        if (resolution.cells.empty())
        {
            _session.breakCombo();
            ensurePlayableBoard();
            _state = BoardStateMachine::onWaitingForInput();
        }
        else
        {
            beginClearAnimation(resolution);
        }
        return;
    }

    _state = BoardStateMachine::onFallsStarted();
}

void PuzzleBoard::finishAnimationStep()
{
    // 현재 상태에 따라 "애니메이션 종료 후 다음 처리"가 달라진다.
    // 이 함수가 사실상 퍼즐 턴 파이프라인의 분기 지점이다.
    switch (_state)
    {
    case BoardState::AnimatingSwap:
    {
        // 특수 조합이 예약되어 있으면 그 결과를 우선 사용하고,
        // 아니면 일반 보드 매치를 계산한다.
        const MatchResolution resolution = !_pendingSwapResolution.cells.empty()
            ? _pendingSwapResolution
            : MatchFactory::createBoardMatch(_tiles, _swapSource, _swapTarget);
        _pendingSwapResolution = {};
        if (resolution.cells.empty())
        {
            _session.breakCombo();
            ensurePlayableBoard();
            _state = BoardStateMachine::onWaitingForInput();
            return;
        }

        beginClearAnimation(resolution);
        return;
    }
    case BoardState::AnimatingInvalidSwapForward:
        // 실패 스왑은 전진 애니메이션 뒤에 반대 방향으로 한 번 더 재생해 원위치시킨다.
        beginSwapAnimation(_swapTarget, _swapSource, BoardStateMachine::onInvalidSwapForwardFinished());
        return;
    case BoardState::AnimatingInvalidSwapBack:
        _state = BoardStateMachine::onInvalidSwapBackFinished();
        return;
    case BoardState::AnimatingClear:
        // 클리어 연출이 끝나면 실제 제거 -> 낙하 단계로 이어진다.
        resolveClearedTiles();
        beginFallAnimation();
        return;
    case BoardState::AnimatingFall:
    {
        // 낙하가 끝났으면 연쇄 매치가 새로 생겼는지 다시 검사한다.
        const MatchResolution resolution = MatchFactory::createBoardMatch(_tiles, _swapSource, _swapTarget);
        if (resolution.cells.empty())
        {
            _session.breakCombo();
            ensurePlayableBoard();
            _state = BoardStateMachine::onWaitingForInput();
        }
        else
        {
            beginClearAnimation(resolution);
        }
        return;
    }
    default:
        return;
    }
}

void PuzzleBoard::updateAnimations(double deltaTime)
{
    // false이면 아직 애니메이션이 진행 중이라는 뜻이고,
    // true이면 방금 한 단계가 끝났으므로 다음 상태로 넘긴다.
    if (!_animator.updateAnimations(deltaTime))
    {
        return;
    }
    finishAnimationStep();
}

void PuzzleBoard::updateEffects(double deltaTime)
{
    _animator.updateEffects(deltaTime);
}
