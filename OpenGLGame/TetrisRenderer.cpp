#include "pch.h"
#include "TetrisRenderer.h"

#include "OpenGLWindow.h"
#include "Shader.h"
#include "Sprite.h"
#include "TetrisHudConfig.h"

#include <filesystem>

namespace
{
    float randomRange(std::mt19937& rng, float minValue, float maxValue)
    {
        std::uniform_real_distribution<float> distribution(minValue, maxValue);
        return distribution(rng);
    }

    std::string resolveAssetPath(const char* filename)
    {
        const std::array<std::string, 3> candidates = {
            std::string(".\\OpenGLGame\\Image\\") + filename,
            std::string(".\\Image\\") + filename,
            std::string(filename)
        };

        for (const auto& candidate : candidates)
        {
            if (std::filesystem::exists(candidate))
            {
                return candidate;
            }
        }

        return candidates.front();
    }
}

void TetrisRenderer::initialize()
{
    if (spriteShader_ != nullptr)
    {
        return;
    }

    const std::string spriteVertexShader = R"(
        #version 330 core
        layout (location = 0) in vec2 aPos;
        layout (location = 1) in vec4 aColor;
        layout (location = 2) in vec2 aTexCoord;
        uniform mat4 projection_matrx;
        uniform mat4 model_matrx;
        uniform float zDepth;
        out vec2 TexCoord;
        void main()
        {
            TexCoord = aTexCoord;
            gl_Position = projection_matrx * model_matrx * vec4(aPos, zDepth, 1.0);
        }
    )";

    const std::string spriteFragmentShader = R"(
        #version 330 core
        out vec4 FragColor;
        in vec2 TexCoord;
        uniform sampler2D imageTexture;
        void main()
        {
            FragColor = texture(imageTexture, TexCoord);
        }
    )";

    spriteShader_ = std::make_shared<Shader>(spriteVertexShader, spriteFragmentShader);
    spriteShader_->use();
    spriteShader_->setInt("imageTexture", 0);

    const std::string panelVertexShader = R"(
        #version 330 core
        layout (location = 0) in vec2 aPos;
        uniform mat4 projection;
        uniform mat4 model;
        void main()
        {
            gl_Position = projection * model * vec4(aPos, 0.0, 1.0);
        }
    )";

    const std::string panelFragmentShader = R"(
        #version 330 core
        out vec4 FragColor;
        uniform vec4 color;
        void main()
        {
            FragColor = color;
        }
    )";

    panelShader_ = std::make_shared<Shader>(panelVertexShader, panelFragmentShader);

    static constexpr float vertices[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };
    static constexpr unsigned int indices[] = { 0, 1, 2, 0, 2, 3 };

    // 패널, 플래시, 오버레이, 파티클은 전부 "색 있는 사각형"만 있으면 그릴 수 있다.
    // 그래서 사각형 메시 하나만 만들어 두고 위치/크기/색만 바꿔 재사용한다.
    glGenVertexArrays(1, &panelVAO_);
    glGenBuffers(1, &panelVBO_);
    glGenBuffers(1, &panelEBO_);
    glBindVertexArray(panelVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, panelVBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, panelEBO_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glBindVertexArray(0);

    const std::array<std::string, 5> paths = {
        resolveAssetPath("red_block.png"),
        resolveAssetPath("blue_block.png"),
        resolveAssetPath("yellow_block.png"),
        resolveAssetPath("green_block.png"),
        resolveAssetPath("purple_block.png")
    };

    for (int i = 0; i < static_cast<int>(paths.size()); ++i)
    {
        blockSprites_[i] = std::make_unique<Sprite>(spriteShader_, paths[i].c_str());
        blockSprites_[i]->SetDepth(0.0f);
    }
}

void TetrisRenderer::release()
{
    if (panelEBO_ != 0) glDeleteBuffers(1, &panelEBO_);
    if (panelVBO_ != 0) glDeleteBuffers(1, &panelVBO_);
    if (panelVAO_ != 0) glDeleteVertexArrays(1, &panelVAO_);
    panelEBO_ = panelVBO_ = panelVAO_ = 0;

    for (auto& sprite : blockSprites_)
    {
        sprite.reset();
    }
    ambientParticles_.clear();
    burstParticles_.clear();
    lastLineClearEventId_ = 0;
    lastHardDropEventId_ = 0;
    lastResetEventId_ = 0;
    spriteShader_.reset();
    panelShader_.reset();
}

void TetrisRenderer::update(const OpenGLWindow& window, const TetrisGame& game, double deltaTime)
{
    if (ambientParticles_.empty())
    {
        spawnAmbientParticles(window);
    }

    const LayoutMetrics layout = getLayoutMetrics(window);
    if (game.getResetEventId() != lastResetEventId_)
    {
        // 새 게임이 시작되면 라인 클리어/하드드롭 같은 일회성 이펙트는 비우고,
        // 배경 파티클은 유지해서 화면 분위기가 갑자기 끊기지 않게 한다.
        lastResetEventId_ = game.getResetEventId();
        burstParticles_.clear();
        lastLineClearEventId_ = game.getLineClearEventId();
        lastHardDropEventId_ = game.getHardDropEventId();
    }

    if (game.getLineClearEventId() != lastLineClearEventId_)
    {
        // 라인 클리어는 실제로 보드 정리가 끝난 뒤 이벤트가 올라온다.
        // 그 시점에 파티클을 생성하면 화면과 효과 타이밍이 가장 자연스럽다.
        lastLineClearEventId_ = game.getLineClearEventId();
        spawnLineClearBurst(layout, game);
    }

    if (game.getHardDropEventId() != lastHardDropEventId_)
    {
        lastHardDropEventId_ = game.getHardDropEventId();
        spawnHardDropBurst(layout, game);
    }

    for (auto& particle : ambientParticles_)
    {
        particle.position += particle.velocity * static_cast<float>(deltaTime);
        particle.life -= static_cast<float>(deltaTime);
        if (particle.position.y > static_cast<float>(window.getScreenHeight()) + particle.size || particle.life <= 0.0f)
        {
            respawnAmbientParticle(particle, window);
        }
    }

    burstParticles_.erase(
        std::remove_if(
            burstParticles_.begin(),
            burstParticles_.end(),
            [deltaTime](Particle& particle)
            {
                particle.position += particle.velocity * static_cast<float>(deltaTime);
                particle.velocity *= 0.985f;
                particle.life -= static_cast<float>(deltaTime);
                return particle.life <= 0.0f;
            }),
        burstParticles_.end());
}

void TetrisRenderer::onWindowSizeChanged(const OpenGLWindow& window) const
{
    updateSpriteProjection(window);
    updatePanelProjection(window);
}

void TetrisRenderer::render(const OpenGLWindow& window, const TetrisGame& game)
{
    const LayoutMetrics layout = getLayoutMetrics(window);
    drawHudBackground(layout);
    drawParticles(layout, true);
    drawBoard(layout, game);
    drawGhostPiece(layout, game);
    drawCurrentPiece(layout, game);
    drawHeldPiecePreview(layout, game);
    drawNextPiecePreview(layout, game);
    drawParticles(layout, false);
    drawHudForeground(layout, game);
}

TetrisRenderer::LayoutMetrics TetrisRenderer::getLayoutMetrics(const OpenGLWindow& window) const
{
    LayoutMetrics layout;
    const float windowWidth = static_cast<float>(window.getScreenWidth());
    const float windowHeight = static_cast<float>(window.getScreenHeight());

    // 전체 UI는 셀 크기를 기준으로 잡는다.
    // 창 비율이 바뀌어도 보드, HOLD/NEXT 패널, 점수판이 같은 비례로 커지게 하려는 의도다.
    layout.cellSize = std::min(windowWidth * 0.042f, windowHeight * 0.043f);
    layout.boardWidthPixels = layout.cellSize * TetrisGame::BoardWidth;
    layout.boardHeightPixels = layout.cellSize * TetrisGame::BoardHeight;
    layout.originX = std::max(28.0f, (windowWidth - layout.boardWidthPixels) * 0.28f);
    layout.originY = (windowHeight - layout.boardHeightPixels) * 0.5f;
    layout.previewSize = layout.cellSize * 4.8f;
    layout.previewX = layout.originX + layout.boardWidthPixels + layout.cellSize * 1.2f;
    layout.previewY = layout.originY + layout.boardHeightPixels - layout.previewSize;
    layout.holdSize = layout.previewSize;
    layout.holdX = std::max(layout.cellSize * 0.9f, layout.originX - layout.holdSize - layout.cellSize * 1.2f);
    layout.holdY = layout.previewY;
    layout.scorePanelX = layout.previewX;
    layout.scorePanelWidth = layout.previewSize;
    layout.scorePanelHeight = layout.cellSize * 3.6f;
    layout.scorePanelY = layout.previewY - layout.cellSize * 4.6f;
    layout.linesPanelY = layout.scorePanelY - layout.cellSize * 4.1f;
    layout.levelPanelY = layout.linesPanelY - layout.cellSize * 4.1f;
    layout.comboPanelY = layout.levelPanelY - layout.cellSize * 4.1f;
    return layout;
}

void TetrisRenderer::updateSpriteProjection(const OpenGLWindow& window) const
{
    if (spriteShader_ == nullptr) return;
    spriteShader_->use();
    spriteShader_->setMat4("projection_matrx", window.getOrthoProjectionMatrix());
}

void TetrisRenderer::updatePanelProjection(const OpenGLWindow& window) const
{
    if (panelShader_ == nullptr) return;
    panelShader_->use();
    panelShader_->setMat4("projection", window.getOrthoProjectionMatrix());
}

int TetrisRenderer::getSpriteIndexForShape(int shape) const
{
    static constexpr std::array<int, 7> shapeToSprite = { 1, 2, 4, 3, 0, 1, 2 };
    return shapeToSprite[shape];
}

void TetrisRenderer::drawHudPanel(const PanelStyle& panel, float borderPadding, float headerHeight) const
{
    drawPanelRect(panel.x - borderPadding, panel.y - borderPadding, panel.width + borderPadding * 2.0f, panel.height + borderPadding * 2.0f, glm::vec4(0.10f, 0.12f, 0.18f, 1.0f));
    drawPanelRect(panel.x, panel.y, panel.width, panel.height, glm::vec4(0.08f, 0.10f, 0.15f, 1.0f));
    drawPanelRect(panel.x, panel.y + panel.height - headerHeight, panel.width, headerHeight, glm::vec4(panel.accent, 1.0f));
}

void TetrisRenderer::drawPanelRect(float x, float y, float width, float height, const glm::vec4& color) const
{
    if (panelShader_ == nullptr || panelVAO_ == 0) return;
    glm::mat4 model(1.0f);
    model = glm::translate(model, glm::vec3(x, y, 0.0f));
    model = glm::scale(model, glm::vec3(width, height, 1.0f));
    panelShader_->use();
    panelShader_->setMat4("model", model);
    panelShader_->setVec4("color", color);
    glBindVertexArray(panelVAO_);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void TetrisRenderer::drawBlockSprite(int spriteIndex, float x, float y, float width, float height)
{
    if (spriteIndex < 0 || spriteIndex >= static_cast<int>(blockSprites_.size())) return;
    auto& sprite = blockSprites_[spriteIndex];
    if (sprite == nullptr) return;
    sprite->SetPosition(x, y);
    sprite->SetScale(width, height);
    sprite->Draw();
}

void TetrisRenderer::drawDigit(int digit, float x, float y, float unit, int spriteIndex)
{
    if (digit < 0 || digit > 9) return;
    const float horizontalWidth = unit * 2.0f;
    const float verticalHeight = unit * 2.0f;
    const float rightX = x + unit * 2.0f;
    const float middleY = y + unit * 2.0f;
    const float topY = y + unit * 4.0f;
    const auto& seg = tetris::hud::DigitSegments[digit];
    if (seg[0]) drawBlockSprite(spriteIndex, x, topY, horizontalWidth, unit);
    if (seg[1]) drawBlockSprite(spriteIndex, rightX, middleY, unit, verticalHeight);
    if (seg[2]) drawBlockSprite(spriteIndex, rightX, y, unit, verticalHeight);
    if (seg[3]) drawBlockSprite(spriteIndex, x, y, horizontalWidth, unit);
    if (seg[4]) drawBlockSprite(spriteIndex, x - unit, y, unit, verticalHeight);
    if (seg[5]) drawBlockSprite(spriteIndex, x - unit, middleY, unit, verticalHeight);
    if (seg[6]) drawBlockSprite(spriteIndex, x, middleY, horizontalWidth, unit);
}

void TetrisRenderer::drawNumber(int number, int minDigits, float x, float y, float unit, int spriteIndex)
{
    const std::string text = std::to_string(std::max(0, number));
    const int totalDigits = std::max(minDigits, static_cast<int>(text.size()));
    const float advance = unit * 4.6f;
    for (int index = 0; index < totalDigits; ++index)
    {
        const int textIndex = static_cast<int>(text.size()) - totalDigits + index;
        const int digit = textIndex >= 0 ? (text[textIndex] - '0') : 0;
        drawDigit(digit, x + advance * index + unit, y, unit, spriteIndex);
    }
}

void TetrisRenderer::drawPieceBlocks(const TetrisGame::ActivePiece& piece, float baseX, float baseY, float cellSize, bool boardCoordinates)
{
    for (int row = 0; row < 4; ++row)
    {
        for (int column = 0; column < 4; ++column)
        {
            if (!TetrisGame::isFilledCell(piece.shape, piece.rotation, row, column)) continue;
            const float cellX = baseX + (piece.x + column) * cellSize;
            const int yIndex = piece.y + row;
            // HOLD/NEXT 미리보기는 4x4 로컬 공간 그대로 쓰고,
            // 메인 보드는 행 인덱스를 화면 좌표계에 맞게 뒤집어서 아래에서 위로 쌓이게 그린다.
            const float cellY = baseY + (boardCoordinates ? (TetrisGame::BoardHeight - 1 - yIndex) * cellSize : (3 - row) * cellSize);
            drawBlockSprite(getSpriteIndexForShape(piece.shape), cellX + tetris::hud::CellPadding, cellY + tetris::hud::CellPadding, cellSize - tetris::hud::CellPadding * 2.0f, cellSize - tetris::hud::CellPadding * 2.0f);
        }
    }
}

void TetrisRenderer::drawLabel(const std::string& text, float x, float y, float unit, const glm::vec3& color) const
{
    float cursorX = x;
    for (char raw : text)
    {
        const char ch = static_cast<char>(std::toupper(static_cast<unsigned char>(raw)));
        const auto found = tetris::hud::Glyphs.find(ch);
        const auto& glyph = found != tetris::hud::Glyphs.end() ? found->second : tetris::hud::Glyphs.at(' ');
        for (int row = 0; row < 5; ++row)
        {
            for (int column = 0; column < 5; ++column)
            {
                if (glyph[row][column] == '1')
                {
                    drawPanelRect(cursorX + column * unit, y + (4 - row) * unit, unit * 0.9f, unit * 0.9f, glm::vec4(color, 1.0f));
                }
            }
        }
        cursorX += unit * 6.0f;
    }
}

void TetrisRenderer::drawHudBackground(const LayoutMetrics& layout)
{
    const float outerPad = layout.cellSize * 0.34f;
    const float innerPad = layout.cellSize * 0.14f;
    const float hudBorder = outerPad * 0.4f;
    const float hudHeader = layout.cellSize * 0.28f;
    drawPanelRect(layout.originX - outerPad, layout.originY - outerPad, layout.boardWidthPixels + outerPad * 2.0f, layout.boardHeightPixels + outerPad * 2.0f, glm::vec4(0.10f, 0.12f, 0.18f, 1.0f));
    drawPanelRect(layout.originX - innerPad, layout.originY - innerPad, layout.boardWidthPixels + innerPad * 2.0f, layout.boardHeightPixels + innerPad * 2.0f, glm::vec4(0.18f, 0.22f, 0.33f, 1.0f));
    drawHudPanel({ layout.holdX, layout.previewY, layout.holdSize, layout.holdSize, glm::vec3(0.93f, 0.42f, 0.33f) }, hudBorder, hudHeader);
    drawHudPanel({ layout.previewX, layout.previewY, layout.previewSize, layout.previewSize, glm::vec3(0.28f, 0.55f, 0.93f) }, hudBorder, hudHeader);
    drawHudPanel({ layout.scorePanelX, layout.scorePanelY, layout.scorePanelWidth, layout.scorePanelHeight, glm::vec3(0.92f, 0.75f, 0.25f) }, hudBorder, hudHeader);
    drawHudPanel({ layout.scorePanelX, layout.linesPanelY, layout.scorePanelWidth, layout.scorePanelHeight, glm::vec3(0.27f, 0.82f, 0.46f) }, hudBorder, hudHeader);
    drawHudPanel({ layout.scorePanelX, layout.levelPanelY, layout.scorePanelWidth, layout.scorePanelHeight, glm::vec3(0.71f, 0.38f, 0.89f) }, hudBorder, hudHeader);
    drawHudPanel({ layout.scorePanelX, layout.comboPanelY, layout.scorePanelWidth, layout.scorePanelHeight, glm::vec3(0.95f, 0.44f, 0.34f) }, hudBorder, hudHeader);
}

void TetrisRenderer::drawHudForeground(const LayoutMetrics& layout, const TetrisGame& game)
{
    const float digitUnit = std::max(4.0f, layout.cellSize * 0.22f);
    const float labelUnit = std::max(3.0f, layout.cellSize * 0.11f);
    drawLabel(tetris::hud::HoldLabel, layout.holdX + layout.cellSize * 0.55f, layout.holdY + layout.holdSize - layout.cellSize * 0.82f, labelUnit, glm::vec3(0.98f, 0.82f, 0.72f));
    drawLabel(tetris::hud::NextLabel, layout.previewX + layout.cellSize * 0.55f, layout.previewY + layout.previewSize - layout.cellSize * 0.82f, labelUnit, glm::vec3(0.80f, 0.88f, 0.98f));
    drawLabel(tetris::hud::ScoreLabel, layout.scorePanelX + layout.cellSize * 0.3f, layout.scorePanelY + layout.scorePanelHeight - layout.cellSize * 0.75f, labelUnit, glm::vec3(0.98f, 0.89f, 0.47f));
    drawLabel(tetris::hud::LinesLabel, layout.scorePanelX + layout.cellSize * 0.45f, layout.linesPanelY + layout.scorePanelHeight - layout.cellSize * 0.75f, labelUnit, glm::vec3(0.63f, 0.96f, 0.67f));
    drawLabel(tetris::hud::LevelLabel, layout.scorePanelX + layout.cellSize * 0.45f, layout.levelPanelY + layout.scorePanelHeight - layout.cellSize * 0.75f, labelUnit, glm::vec3(0.89f, 0.70f, 0.99f));
    drawLabel(tetris::hud::ComboLabel, layout.scorePanelX + layout.cellSize * 0.35f, layout.comboPanelY + layout.scorePanelHeight - layout.cellSize * 0.75f, labelUnit, glm::vec3(0.98f, 0.74f, 0.65f));
    drawNumber(game.getScore(), 5, layout.scorePanelX + layout.cellSize * 0.35f, layout.scorePanelY + layout.cellSize * 0.55f, digitUnit, 2);
    drawNumber(game.getLinesCleared(), 3, layout.scorePanelX + layout.cellSize * 0.85f, layout.linesPanelY + layout.cellSize * 0.55f, digitUnit, 3);
    drawNumber(game.getLevel(), 2, layout.scorePanelX + layout.cellSize * 1.35f, layout.levelPanelY + layout.cellSize * 0.55f, digitUnit, 4);
    drawNumber(game.getComboCount(), 2, layout.scorePanelX + layout.cellSize * 1.35f, layout.comboPanelY + layout.cellSize * 0.55f, digitUnit, 0);
    if (game.isPaused() || game.isGameOver())
    {
        const float overlayWidth = layout.boardWidthPixels * 0.78f;
        const float overlayHeight = layout.cellSize * 4.2f;
        const float overlayX = layout.originX + (layout.boardWidthPixels - overlayWidth) * 0.5f;
        const float overlayY = layout.originY + (layout.boardHeightPixels - overlayHeight) * 0.5f;
        const glm::vec4 overlayColor = game.isGameOver() ? glm::vec4(0.10f, 0.05f, 0.08f, 0.92f) : glm::vec4(0.05f, 0.08f, 0.12f, 0.88f);
        const glm::vec4 headerColor = game.isGameOver() ? glm::vec4(0.84f, 0.23f, 0.30f, 1.0f) : glm::vec4(0.28f, 0.55f, 0.93f, 1.0f);
        drawPanelRect(overlayX, overlayY, overlayWidth, overlayHeight, overlayColor);
        drawPanelRect(overlayX, overlayY + overlayHeight - layout.cellSize * 0.35f, overlayWidth, layout.cellSize * 0.35f, headerColor);
        drawLabel(game.isGameOver() ? tetris::hud::GameOverLabel : tetris::hud::PausedLabel, overlayX + layout.cellSize * 0.65f, overlayY + layout.cellSize * 2.4f, labelUnit * 0.95f, glm::vec3(0.98f, 0.84f, 0.84f));
        drawNumber(game.getScore(), 5, overlayX + layout.cellSize * 1.0f, overlayY + layout.cellSize * 1.15f, digitUnit, 0);
    }
}

void TetrisRenderer::drawParticles(const LayoutMetrics& layout, bool ambientPass)
{
    (void)layout;

    const auto renderParticleList = [&](const std::vector<Particle>& particles)
    {
        for (const auto& particle : particles)
        {
            const float lifeRatio = particle.maxLife > 0.0f ? std::clamp(particle.life / particle.maxLife, 0.0f, 1.0f) : 0.0f;
            glm::vec4 color = particle.color;
            color.a *= ambientPass ? lifeRatio * 0.6f : lifeRatio;
            drawPanelRect(
                particle.position.x,
                particle.position.y,
                particle.size,
                particle.size,
                color);
        }
    };

    if (ambientPass)
    {
        // 배경 파티클은 먼저 그려서 분위기만 살리고,
        // 버스트 파티클은 나중에 그려 충돌/클리어 연출이 보드 위로 튀어나오게 만든다.
        renderParticleList(ambientParticles_);
        return;
    }

    renderParticleList(burstParticles_);
}

void TetrisRenderer::drawBoard(const LayoutMetrics& layout, const TetrisGame& game)
{
    drawPanelRect(layout.originX, layout.originY, layout.boardWidthPixels, layout.boardHeightPixels, glm::vec4(0.06f, 0.08f, 0.12f, 1.0f));
    const auto& board = game.getBoard();
    for (int row = 0; row < TetrisGame::BoardHeight; ++row)
    {
        for (int column = 0; column < TetrisGame::BoardWidth; ++column)
        {
            const float cellX = layout.originX + column * layout.cellSize;
            const float cellY = layout.originY + (TetrisGame::BoardHeight - 1 - row) * layout.cellSize;
            const float inset = std::max(1.0f, layout.cellSize * 0.04f);
            const glm::vec4 cellColor = ((row + column) % 2 == 0)
                ? glm::vec4(0.09f, 0.11f, 0.16f, 1.0f)
                : glm::vec4(0.11f, 0.13f, 0.19f, 1.0f);
            drawPanelRect(cellX + inset, cellY + inset, layout.cellSize - inset * 2.0f, layout.cellSize - inset * 2.0f, cellColor);
            const int cell = board[row][column];
            if (cell != 0)
            {
                drawBlockSprite(getSpriteIndexForShape(cell - 1), cellX + tetris::hud::CellPadding, cellY + tetris::hud::CellPadding, layout.cellSize - tetris::hud::CellPadding * 2.0f, layout.cellSize - tetris::hud::CellPadding * 2.0f);
            }

            if (game.getClearingRows()[row])
            {
                // 줄 삭제 깜빡임은 블록 텍스처를 바꾸지 않고 반투명 오버레이로 덮는다.
                // 그래서 어떤 색 블록이든 같은 연출을 손쉽게 적용할 수 있다.
                const float flash = static_cast<float>(0.35 + 0.65 * std::sin(game.getLineClearAnimationProgress() * 18.0));
                drawPanelRect(
                    cellX + inset,
                    cellY + inset,
                    layout.cellSize - inset * 2.0f,
                    layout.cellSize - inset * 2.0f,
                    glm::vec4(1.0f, 0.96f, 0.72f, flash * 0.85f));
            }
        }
    }
}

void TetrisRenderer::drawGhostPiece(const LayoutMetrics& layout, const TetrisGame& game)
{
    if (game.isGameOver() || game.isPaused() || game.isAnimatingLineClear())
    {
        return;
    }

    const auto ghost = game.getGhostPiece();
    const auto& current = game.getCurrentPiece();
    if (ghost.y == current.y)
    {
        return;
    }

    const float inset = std::max(2.0f, layout.cellSize * 0.18f);
    const glm::vec4 ghostColor(0.92f, 0.96f, 1.0f, 0.28f);
    for (int row = 0; row < 4; ++row)
    {
        for (int column = 0; column < 4; ++column)
        {
            if (!TetrisGame::isFilledCell(ghost.shape, ghost.rotation, row, column))
            {
                continue;
            }

            const float cellX = layout.originX + (ghost.x + column) * layout.cellSize;
            const float cellY = layout.originY + (TetrisGame::BoardHeight - 1 - (ghost.y + row)) * layout.cellSize;
            drawPanelRect(cellX + inset, cellY + inset, layout.cellSize - inset * 2.0f, layout.cellSize - inset * 2.0f, ghostColor);
        }
    }
}

void TetrisRenderer::drawCurrentPiece(const LayoutMetrics& layout, const TetrisGame& game)
{
    if (game.isGameOver() || game.isPaused() || game.isAnimatingLineClear()) return;
    drawPieceBlocks(game.getCurrentPiece(), layout.originX, layout.originY, layout.cellSize, true);
}

void TetrisRenderer::drawHeldPiecePreview(const LayoutMetrics& layout, const TetrisGame& game)
{
    drawPanelRect(layout.holdX, layout.holdY, layout.holdSize, layout.holdSize, glm::vec4(0.07f, 0.09f, 0.14f, 1.0f));
    if (!game.hasHeldPiece())
    {
        return;
    }

    TetrisGame::ActivePiece heldPiece{};
    heldPiece.shape = game.getHeldShape();
    heldPiece.rotation = 0;
    heldPiece.x = 0;
    heldPiece.y = 0;
    drawPieceBlocks(heldPiece, layout.holdX, layout.holdY, layout.cellSize, false);
}

void TetrisRenderer::drawNextPiecePreview(const LayoutMetrics& layout, const TetrisGame& game)
{
    drawPanelRect(layout.previewX, layout.previewY, layout.previewSize, layout.previewSize, glm::vec4(0.07f, 0.09f, 0.14f, 1.0f));
    drawPieceBlocks(game.getNextPiece(), layout.previewX, layout.previewY, layout.cellSize, false);
}

void TetrisRenderer::spawnAmbientParticles(const OpenGLWindow& window)
{
    // 배경 파티클은 가벼운 사각형 기반으로 유지한다.
    // 별도 텍스처 자산 없이도 화면이 덜 밋밋해지고, 성능 부담도 거의 없다.
    ambientParticles_.resize(36);
    for (auto& particle : ambientParticles_)
    {
        respawnAmbientParticle(particle, window);
        particle.position.y = randomRange(particleRng_, 0.0f, static_cast<float>(window.getScreenHeight()));
    }
}

void TetrisRenderer::respawnAmbientParticle(Particle& particle, const OpenGLWindow& window)
{
    particle.size = randomRange(particleRng_, 3.0f, 10.0f);
    particle.position.x = randomRange(particleRng_, -40.0f, static_cast<float>(window.getScreenWidth()) + 20.0f);
    particle.position.y = -particle.size - randomRange(particleRng_, 0.0f, 180.0f);
    particle.velocity = glm::vec2(randomRange(particleRng_, -10.0f, 10.0f), randomRange(particleRng_, 18.0f, 44.0f));
    particle.color = glm::vec4(
        randomRange(particleRng_, 0.16f, 0.28f),
        randomRange(particleRng_, 0.22f, 0.40f),
        randomRange(particleRng_, 0.32f, 0.58f),
        randomRange(particleRng_, 0.18f, 0.36f));
    particle.maxLife = randomRange(particleRng_, 8.0f, 16.0f);
    particle.life = particle.maxLife;
}

void TetrisRenderer::spawnLineClearBurst(const LayoutMetrics& layout, const TetrisGame& game)
{
    // 실제로 삭제된 줄 좌표를 기준으로 버스트를 뿌린다.
    // 보드가 아래로 당겨진 뒤에도 "어디서 터졌는지"가 어긋나지 않게 하기 위한 처리다.
    for (int index = 0; index < game.getLastClearedRowCount(); ++index)
    {
        const int row = game.getLastClearedRows()[index];
        if (row < 0)
        {
            continue;
        }

        const float y = layout.originY + (TetrisGame::BoardHeight - 1 - row) * layout.cellSize + layout.cellSize * 0.25f;
        for (int particleIndex = 0; particleIndex < 18; ++particleIndex)
        {
            Particle particle;
            particle.position = glm::vec2(
                layout.originX + randomRange(particleRng_, 0.0f, layout.boardWidthPixels),
                y + randomRange(particleRng_, -8.0f, 8.0f));
            particle.velocity = glm::vec2(
                randomRange(particleRng_, -80.0f, 80.0f),
                randomRange(particleRng_, -45.0f, 45.0f));
            particle.size = randomRange(particleRng_, layout.cellSize * 0.12f, layout.cellSize * 0.32f);
            particle.color = glm::vec4(
                randomRange(particleRng_, 0.92f, 1.0f),
                randomRange(particleRng_, 0.70f, 0.95f),
                randomRange(particleRng_, 0.35f, 0.65f),
                0.95f);
            particle.maxLife = randomRange(particleRng_, 0.30f, 0.62f);
            particle.life = particle.maxLife;
            burstParticles_.push_back(particle);
        }
    }
}

void TetrisRenderer::spawnHardDropBurst(const LayoutMetrics& layout, const TetrisGame& game)
{
    // 하드드롭은 이미 현재 피스가 사라진 뒤일 수 있으므로,
    // 마지막으로 고정된 피스 정보를 사용해 착지 위치에 정확히 이펙트를 생성한다.
    const auto& landingPiece = game.getLastLockedPiece();
    for (int row = 0; row < 4; ++row)
    {
        for (int column = 0; column < 4; ++column)
        {
            if (!TetrisGame::isFilledCell(landingPiece.shape, landingPiece.rotation, row, column))
            {
                continue;
            }

            const float x = layout.originX + (landingPiece.x + column) * layout.cellSize + layout.cellSize * 0.3f;
            const float y = layout.originY + (TetrisGame::BoardHeight - 1 - (landingPiece.y + row)) * layout.cellSize + layout.cellSize * 0.12f;
            for (int particleIndex = 0; particleIndex < 6; ++particleIndex)
            {
                Particle particle;
                particle.position = glm::vec2(x, y);
                particle.velocity = glm::vec2(randomRange(particleRng_, -45.0f, 45.0f), randomRange(particleRng_, -110.0f, -20.0f));
                particle.size = randomRange(particleRng_, layout.cellSize * 0.10f, layout.cellSize * 0.22f);
                particle.color = glm::vec4(0.78f, 0.88f, 1.0f, 0.85f);
                particle.maxLife = randomRange(particleRng_, 0.22f, 0.45f);
                particle.life = particle.maxLife;
                burstParticles_.push_back(particle);
            }
        }
    }
}
