#pragma once

#ifndef PUZZLERENDERER_H_
#define PUZZLERENDERER_H_

#include "Shader.h"
#include "VertexBuffer2D.h"
#include "PuzzleTypes.h"

// 퍼즐 보드의 배경, 일반 타일, 애니메이션 타일, 이펙트를 실제 OpenGL로 그리는 렌더러다.
class PuzzleRenderer
{
public:
    PuzzleRenderer();
    ~PuzzleRenderer();

    void initialize();
    void resize(int screenWidth, int screenHeight);
    void render(
        const mat4& orthoProjection,
        const PuzzleGrid& tiles,
        const vector<TileAnimation>& animations,
        const vector<EffectBurst>& effects,
        bool hasSelection,
        const Cell& selectedCell,
        BoardState state,
        float gameOverPulse);

    vec2 getCellPosition(const Cell& cell) const;
    Cell toCell(const ivec2& cursorPosition) const;

private:
    // 화면 크기에 따라 보드 위치와 셀 크기를 다시 계산해 반응형처럼 맞춘다.
    shared_ptr<Shader> _shader;
    VertexBufferObject2D _quad{};
    array<vec4, PuzzleTileTypeCount> _tileColors{};

    int _screenWidth = 800;
    int _screenHeight = 600;
    float _boardLeft = 120.0f;
    float _boardBottom = 60.0f;
    float _cellSize = 56.0f;
    float _tilePadding = 6.0f;
    bool _initialized = false;

private:
    // 보드 정적 타일과 애니메이션 타일이 겹쳐 그려지지 않도록 숨김 여부를 계산한다.
    bool isCellHiddenByAnimation(const vector<TileAnimation>& animations, int row, int column) const;
    // 선택 상태를 반영해 최종 타일 색을 만든다.
    vec4 colorForTile(const Tile& tile, bool isSelected) const;
    // 보드 뒤쪽 패널 배경을 그린다.
    void drawBoardBackground(const mat4& orthoProjection);
    // 제거/특수 발동 이펙트를 그린다.
    void drawEffects(const mat4& orthoProjection, const vector<EffectBurst>& effects);
    // 타일 본체와 특수 블록 마커를 함께 그린다.
    void drawTile(const mat4& orthoProjection, const Tile& tile, float x, float y, bool isSelected, float alpha = 1.0f);
    // 내부 공용 사각형 드로우 헬퍼다.
    void drawRect(const mat4& orthoProjection, float x, float y, float width, float height, const vec4& color);
};

#endif // !PUZZLERENDERER_H_
