#pragma once

#ifndef TETRISRENDERER_H_
#define TETRISRENDERER_H_

#include "pch.h"
#include "TetrisGame.h"
#include "Sprite.h"

class Shader;
class OpenGLWindow;

class TetrisRenderer
{
public:
    void initialize();
    void release();
    void update(const OpenGLWindow& window, const TetrisGame& game, double deltaTime);
    void onWindowSizeChanged(const OpenGLWindow& window) const;
    void render(const OpenGLWindow& window, const TetrisGame& game);

private:
    struct LayoutMetrics
    {
        float cellSize = 0.0f;
        float originX = 0.0f;
        float originY = 0.0f;
        float boardWidthPixels = 0.0f;
        float boardHeightPixels = 0.0f;
        float previewX = 0.0f;
        float previewY = 0.0f;
        float previewSize = 0.0f;
        float holdX = 0.0f;
        float holdY = 0.0f;
        float holdSize = 0.0f;
        float scorePanelX = 0.0f;
        float scorePanelY = 0.0f;
        float scorePanelWidth = 0.0f;
        float scorePanelHeight = 0.0f;
        float linesPanelY = 0.0f;
        float levelPanelY = 0.0f;
        float comboPanelY = 0.0f;
    };

    struct PanelStyle
    {
        float x = 0.0f;
        float y = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
        glm::vec3 accent = glm::vec3(1.0f);
    };

    struct Particle
    {
        glm::vec2 position = glm::vec2(0.0f);
        glm::vec2 velocity = glm::vec2(0.0f);
        glm::vec4 color = glm::vec4(1.0f);
        float size = 0.0f;
        float life = 0.0f;
        float maxLife = 1.0f;
    };

private:
    std::shared_ptr<Shader> spriteShader_;
    std::shared_ptr<Shader> panelShader_;
    std::array<std::unique_ptr<Sprite>, 5> blockSprites_{};
    std::vector<Particle> ambientParticles_;
    std::vector<Particle> burstParticles_;
    unsigned int panelVAO_ = 0;
    unsigned int panelVBO_ = 0;
    unsigned int panelEBO_ = 0;
    uint64_t lastLineClearEventId_ = 0;
    uint64_t lastHardDropEventId_ = 0;
    uint64_t lastResetEventId_ = 0;
    std::mt19937 particleRng_{ std::random_device{}() };

private:
    LayoutMetrics getLayoutMetrics(const OpenGLWindow& window) const;
    void updateSpriteProjection(const OpenGLWindow& window) const;
    void updatePanelProjection(const OpenGLWindow& window) const;
    int getSpriteIndexForShape(int shape) const;
    void drawHudPanel(const PanelStyle& panel, float borderPadding, float headerHeight) const;
    void drawPanelRect(float x, float y, float width, float height, const glm::vec4& color) const;
    void drawBlockSprite(int spriteIndex, float x, float y, float width, float height);
    void drawDigit(int digit, float x, float y, float unit, int spriteIndex);
    void drawNumber(int number, int minDigits, float x, float y, float unit, int spriteIndex);
    void drawPieceBlocks(const TetrisGame::ActivePiece& piece, float baseX, float baseY, float cellSize, bool boardCoordinates);
    void drawLabel(const std::string& text, float x, float y, float unit, const glm::vec3& color) const;
    void drawHudBackground(const LayoutMetrics& layout);
    void drawHudForeground(const LayoutMetrics& layout, const TetrisGame& game);
    void drawParticles(const LayoutMetrics& layout, bool ambientPass);
    void drawBoard(const LayoutMetrics& layout, const TetrisGame& game);
    void drawGhostPiece(const LayoutMetrics& layout, const TetrisGame& game);
    void drawCurrentPiece(const LayoutMetrics& layout, const TetrisGame& game);
    void drawHeldPiecePreview(const LayoutMetrics& layout, const TetrisGame& game);
    void drawNextPiecePreview(const LayoutMetrics& layout, const TetrisGame& game);
    void spawnAmbientParticles(const OpenGLWindow& window);
    void respawnAmbientParticle(Particle& particle, const OpenGLWindow& window);
    void spawnLineClearBurst(const LayoutMetrics& layout, const TetrisGame& game);
    void spawnHardDropBurst(const LayoutMetrics& layout, const TetrisGame& game);
};

#endif // !TETRISRENDERER_H_
