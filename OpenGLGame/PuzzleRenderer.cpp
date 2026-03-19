#include "pch.h"
#include "PuzzleRenderer.h"
#include "PuzzleRuleEngine.h"

namespace
{
    // 퍼즐 보드는 단색 사각형 기반의 단순 셰이더만 있으면 충분하다.
    constexpr const char* kPuzzleVertexShader = R"(
        #version 460 core
        layout (location = 0) in vec2 inPosition;

        uniform mat4 projection;
        uniform mat4 model;

        void main()
        {
            gl_Position = projection * model * vec4(inPosition, 0.0, 1.0);
        }
    )";

    constexpr const char* kPuzzleFragmentShader = R"(
        #version 460 core
        out vec4 fragmentColor;

        uniform vec4 tintColor;

        void main()
        {
            fragmentColor = tintColor;
        }
    )";
}

PuzzleRenderer::PuzzleRenderer()
{
    // 타일 색상은 한눈에 구분되도록 채도가 다른 팔레트로 고정한다.
    _tileColors = {
        vec4(0.95f, 0.36f, 0.33f, 1.0f),
        vec4(0.97f, 0.74f, 0.25f, 1.0f),
        vec4(0.32f, 0.75f, 0.43f, 1.0f),
        vec4(0.28f, 0.62f, 0.96f, 1.0f),
        vec4(0.70f, 0.45f, 0.93f, 1.0f)
    };
}

PuzzleRenderer::~PuzzleRenderer()
{
    if (_initialized)
    {
        VertexBufferSystem2D::Delete(_quad);
    }

    if (_shader != nullptr)
    {
        glDeleteProgram(_shader->ID);
    }
}

void PuzzleRenderer::initialize()
{
    if (_initialized)
    {
        return;
    }

    // 정사각형 하나를 반복 재사용해서 보드의 모든 사각형을 그린다.
    _shader = make_shared<Shader>(string(kPuzzleVertexShader), string(kPuzzleFragmentShader));
    _quad = VertexBufferSystem2D::Generate();
    _initialized = true;
}

void PuzzleRenderer::resize(int screenWidth, int screenHeight)
{
    _screenWidth = screenWidth;
    _screenHeight = screenHeight;

    // 보드는 화면 중앙에 최대한 크게 배치하되, 상단 UI 여백을 조금 남긴다.
    const float boardSize = std::min(static_cast<float>(_screenWidth), static_cast<float>(_screenHeight) - 120.0f);
    _cellSize = std::max(40.0f, floorf(boardSize / static_cast<float>(PuzzleColumnCount)));
    const float totalBoardWidth = _cellSize * static_cast<float>(PuzzleColumnCount);
    const float totalBoardHeight = _cellSize * static_cast<float>(PuzzleRowCount);

    _boardLeft = floorf((static_cast<float>(_screenWidth) - totalBoardWidth) * 0.5f);
    _boardBottom = floorf((static_cast<float>(_screenHeight) - totalBoardHeight) * 0.5f - 24.0f);
    _boardBottom = std::max(24.0f, _boardBottom);
    _tilePadding = std::max(4.0f, _cellSize * 0.08f);
}

void PuzzleRenderer::render(
    const mat4& orthoProjection,
    const PuzzleGrid& tiles,
    const vector<TileAnimation>& animations,
    const vector<EffectBurst>& effects,
    bool hasSelection,
    const Cell& selectedCell,
    BoardState state,
    float gameOverPulse)
{
    // 정적 보드 -> 움직이는 타일 -> 이펙트 -> 게임오버 오버레이 순서로 그린다.
    drawBoardBackground(orthoProjection);

    for (int row = 0; row < PuzzleRowCount; ++row)
    {
        for (int column = 0; column < PuzzleColumnCount; ++column)
        {
            const Cell cell{ column, row };
            const vec2 position = getCellPosition(cell);
            const bool isSelected = hasSelection && selectedCell.column == column && selectedCell.row == row;

            drawRect(
                orthoProjection,
                position.x + 1.5f,
                position.y + 1.5f,
                _cellSize - 3.0f,
                _cellSize - 3.0f,
                isSelected ? vec4(0.95f, 0.95f, 0.98f, 1.0f) : vec4(0.10f, 0.12f, 0.18f, 1.0f));

            const Tile& tile = tiles[row][column];
            if (PuzzleRuleEngine::isEmpty(tile) || isCellHiddenByAnimation(animations, row, column))
            {
                // 애니메이션 중인 칸은 정적 타일을 숨겨서 두 장이 겹쳐 보이지 않게 한다.
                continue;
            }

            drawTile(orthoProjection, tile, position.x, position.y, isSelected);
        }
    }

    for (const TileAnimation& animation : animations)
    {
        // 직선 이동이 너무 딱딱하지 않도록 간단한 ease-out 곡선을 적용한다.
        const float t = animation.duration > 0.0f ? std::clamp(animation.elapsed / animation.duration, 0.0f, 1.0f) : 1.0f;
        const float easedT = 1.0f - ((1.0f - t) * (1.0f - t));
        const vec2 position = mix(animation.start, animation.end, easedT);
        const float alpha = animation.flash ? (1.0f - t) : 1.0f;
        drawTile(orthoProjection, animation.tile, position.x, position.y, false, alpha);
    }

    drawEffects(orthoProjection, effects);

    if (state == BoardState::TimeUp)
    {
        const float overlayAlpha = 0.28f + (sinf(gameOverPulse * 3.0f) * 0.08f);
        drawRect(
            orthoProjection,
            _boardLeft - 18.0f,
            _boardBottom - 18.0f,
            (_cellSize * PuzzleColumnCount) + 36.0f,
            (_cellSize * PuzzleRowCount) + 36.0f,
            vec4(0.05f, 0.02f, 0.04f, overlayAlpha));
    }
}

vec2 PuzzleRenderer::getCellPosition(const Cell& cell) const
{
    return vec2(
        _boardLeft + (static_cast<float>(cell.column) * _cellSize),
        _boardBottom + (static_cast<float>(cell.row) * _cellSize));
}

Cell PuzzleRenderer::toCell(const ivec2& cursorPosition) const
{
    // 마우스 좌표를 보드 로컬 좌표로 바꾼 뒤 셀 인덱스로 환산한다.
    const float localX = static_cast<float>(cursorPosition.x) - _boardLeft;
    const float localY = static_cast<float>(cursorPosition.y) - _boardBottom;

    if (localX < 0.0f || localY < 0.0f)
    {
        return {};
    }

    return {
        static_cast<int>(localX / _cellSize),
        static_cast<int>(localY / _cellSize)
    };
}

bool PuzzleRenderer::isCellHiddenByAnimation(const vector<TileAnimation>& animations, int row, int column) const
{
    const Cell cell{ column, row };
    const vec2 position = getCellPosition(cell);

    for (const TileAnimation& animation : animations)
    {
        // 애니메이션의 도착 위치가 이 칸이면, 현재 프레임에서는 정적 타일을 숨긴다.
        if (distance(animation.end, position) < 0.01f)
        {
            return true;
        }
    }

    return false;
}

vec4 PuzzleRenderer::colorForTile(const Tile& tile, bool isSelected) const
{
    vec4 color = _tileColors[tile.color];
    if (isSelected)
    {
        color = glm::min(color + vec4(0.18f, 0.18f, 0.18f, 0.0f), vec4(1.0f));
    }
    return color;
}

void PuzzleRenderer::drawBoardBackground(const mat4& orthoProjection)
{
    // 테두리 느낌을 주기 위해 두 겹의 배경판을 그린다.
    const float boardWidth = _cellSize * static_cast<float>(PuzzleColumnCount);
    const float boardHeight = _cellSize * static_cast<float>(PuzzleRowCount);

    drawRect(orthoProjection, _boardLeft - 18.0f, _boardBottom - 18.0f, boardWidth + 36.0f, boardHeight + 36.0f, vec4(0.13f, 0.16f, 0.24f, 1.0f));
    drawRect(orthoProjection, _boardLeft - 6.0f, _boardBottom - 6.0f, boardWidth + 12.0f, boardHeight + 12.0f, vec4(0.21f, 0.25f, 0.36f, 1.0f));
}

void PuzzleRenderer::drawEffects(const mat4& orthoProjection, const vector<EffectBurst>& effects)
{
    for (const EffectBurst& effect : effects)
    {
        // 시간이 지날수록 커지고 투명해지는 단순 확산형 이펙트다.
        const float t = effect.duration > 0.0f ? std::clamp(effect.elapsed / effect.duration, 0.0f, 1.0f) : 1.0f;
        const float scale = 0.35f + (t * 0.85f);
        vec4 color = effect.color;
        color.a *= (1.0f - t);

        const vec2 size = effect.size * scale;
        drawRect(
            orthoProjection,
            effect.center.x - (size.x * 0.5f),
            effect.center.y - (size.y * 0.5f),
            size.x,
            size.y,
            color);
    }
}

void PuzzleRenderer::drawTile(const mat4& orthoProjection, const Tile& tile, float x, float y, bool isSelected, float alpha)
{
    // 기본 타일 + 상단 하이라이트 + 특수 타입별 마커를 합쳐 한 개 타일을 구성한다.
    vec4 color = colorForTile(tile, isSelected);
    color.a *= alpha;

    drawRect(
        orthoProjection,
        x + _tilePadding,
        y + _tilePadding,
        _cellSize - (_tilePadding * 2.0f),
        _cellSize - (_tilePadding * 2.0f),
        color);

    vec4 shine = glm::min(color + vec4(0.16f, 0.16f, 0.16f, 0.0f), vec4(1.0f));
    shine.a = color.a * 0.45f;
    drawRect(
        orthoProjection,
        x + _tilePadding + 5.0f,
        y + (_cellSize * 0.52f),
        (_cellSize - (_tilePadding * 2.0f)) - 10.0f,
        (_cellSize * 0.22f),
        shine);

    if (tile.special == SpecialType::RowClear)
    {
        vec4 stripe(1.0f, 1.0f, 1.0f, alpha * 0.75f);
        drawRect(
            orthoProjection,
            x + _tilePadding + 4.0f,
            y + (_cellSize * 0.44f),
            (_cellSize - (_tilePadding * 2.0f)) - 8.0f,
            _cellSize * 0.10f,
            stripe);
    }
    else if (tile.special == SpecialType::Bomb)
    {
        vec4 core(1.0f, 0.95f, 0.80f, alpha * 0.88f);
        drawRect(
            orthoProjection,
            x + (_cellSize * 0.34f),
            y + (_cellSize * 0.34f),
            _cellSize * 0.32f,
            _cellSize * 0.32f,
            core);
    }
    else if (tile.special == SpecialType::ColorBomb)
    {
        vec4 shell(0.98f, 0.98f, 0.98f, alpha * 0.92f);
        vec4 center(0.18f, 0.20f, 0.26f, alpha * 0.96f);
        drawRect(
            orthoProjection,
            x + (_cellSize * 0.27f),
            y + (_cellSize * 0.27f),
            _cellSize * 0.46f,
            _cellSize * 0.46f,
            shell);
        drawRect(
            orthoProjection,
            x + (_cellSize * 0.34f),
            y + (_cellSize * 0.34f),
            _cellSize * 0.32f,
            _cellSize * 0.32f,
            center);
    }
}

void PuzzleRenderer::drawRect(const mat4& orthoProjection, float x, float y, float width, float height, const vec4& color)
{
    mat4 model = mat4(1.0f);
    model = translate(model, vec3(x, y, 0.0f));
    model = scale(model, vec3(width, height, 1.0f));

    _shader->use();
    _shader->setMat4("projection", orthoProjection);
    _shader->setMat4("model", model);
    _shader->setVec4("tintColor", color);
    _quad.Draw();
}
