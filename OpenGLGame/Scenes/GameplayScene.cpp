#include "pch.h"
#include "GameplayScene.h"

#include "MainWindow.h"
#include "Shader.h"
#include "Sprite.h"
#include "TitleScene.h"

namespace
{
    const string kPauseVertexShader = R"(#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec2 aTexCoord;

out vec4 vertexColor;
out vec2 texCoord;

uniform mat4 projection_matrx;
uniform mat4 model_matrx;
uniform float zDepth;

void main()
{
    gl_Position = projection_matrx * model_matrx * vec4(aPos.xy, zDepth, 1.0);
    vertexColor = aColor;
    texCoord = aTexCoord;
}
)";

    const string kPauseFragmentShader = R"(#version 330 core
in vec4 vertexColor;
in vec2 texCoord;

out vec4 FragColor;

uniform sampler2D tex;
uniform vec4 tintColor;

void main()
{
    FragColor = texture(tex, texCoord) * vertexColor * tintColor;
}
)";

    constexpr int kHudFontSize = 28;
    constexpr float kGameplayWorldWidth = 2560.0f;
    constexpr float kCameraPanSpeed = 720.0f;
    constexpr float kCameraEdgeThreshold = 28.0f;
    constexpr vec3 kHeaderColor(0.95f, 0.90f, 0.72f);
    constexpr vec3 kBodyColor(0.86f, 0.90f, 0.96f);
    constexpr vec3 kResourceColor(0.93f, 0.95f, 0.98f);
    constexpr vec3 kStatusColor(0.82f, 0.88f, 0.96f);
    constexpr vec3 kResultColor(1.0f, 0.90f, 0.68f);
    constexpr float kSelectionLineGap = 20.0f;
    constexpr float kCommandLineGap = 20.0f;
    constexpr float kInfoPanelTextWidth = 176.0f;
    constexpr float kResourcePanelTextWidth = 312.0f;

    string findHudFontPath()
    {
        const array<string, 3> candidates =
        {
            "C:\\Windows\\Fonts\\malgun.ttf",
            "C:\\Windows\\Fonts\\malgunsl.ttf",
            "C:\\Windows\\Fonts\\arial.ttf"
        };

        for (const string& candidate : candidates)
        {
            if (filesystem::exists(candidate))
            {
                return candidate;
            }
        }

        return "";
    }

    void renderTextLines(TextEngine& textEngine, const vector<string>& lines, const vec2& start, float scale, float lineGap, const vec3& color)
    {
        float y = start.y;
        for (const string& line : lines)
        {
            const wstring wide(line.begin(), line.end());
            textEngine.renderText(wide, start.x, y, { scale, color });
            y -= lineGap;
        }
    }

    vector<string> wrapTextToWidth(TextEngine& textEngine, const string& text, float scale, float maxWidth)
    {
        if (text.empty())
        {
            return {};
        }

        istringstream stream(text);
        string word;
        string currentLine;
        vector<string> lines;

        while (stream >> word)
        {
            const string candidate = currentLine.empty() ? word : currentLine + " " + word;
            const wstring wideCandidate(candidate.begin(), candidate.end());
            if (!currentLine.empty() && textEngine.measureText(wideCandidate, scale).x > maxWidth)
            {
                lines.push_back(currentLine);
                currentLine = word;
            }
            else
            {
                currentLine = candidate;
            }
        }

        if (!currentLine.empty())
        {
            lines.push_back(currentLine);
        }

        if (lines.empty())
        {
            lines.push_back(text);
        }

        return lines;
    }
}

void GameplayScene::onEnter(MainWindow& window, GameContext& context)
{
    context.currentSceneName = "GameplayScene";
    _state.reset();
    _cameraX = 0.0f;
    _isPaused = false;
    _hoveredPauseAction = PauseMenuAction::None;

    glClearColor(0.08f, 0.11f, 0.14f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    const string fontPath = findHudFontPath();
    _textReady = !fontPath.empty() && _textEngine.initialize(fontPath, kHudFontSize);
    initializePauseArt();
    _state.setViewportSize(window.getScreenWidth(), window.getScreenHeight());

    onResize(window, context, window.getScreenWidth(), window.getScreenHeight());
}

void GameplayScene::onExit(MainWindow& window, GameContext& context)
{
    (void)window;
    (void)context;

    _textEngine.shutdown();
    _textReady = false;
    _pauseShader.reset();
    _pauseBackdrop.reset();
    _pauseWindow.reset();
    _pauseHeader.reset();
    _pauseResumeButton.reset();
    _pauseTitleButton.reset();
}

void GameplayScene::update(MainWindow& window, GameContext& context, double deltaTime)
{
    const vec2 cursorScreenPosition = vec2(window.getOpenGLCursorPosition());
    _hoveredPauseAction = hitTestPauseMenu(cursorScreenPosition);

    if (context.input.wasKeyPressed(GLFW_KEY_F10))
    {
        _isPaused = !_isPaused;
    }

    if (_isPaused)
    {
        context.currentSceneName = "GameplayScene | Paused";

        if (context.input.wasMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT))
        {
            if (_hoveredPauseAction == PauseMenuAction::Resume)
            {
                _isPaused = false;
            }
            else if (_hoveredPauseAction == PauseMenuAction::ReturnToTitle)
            {
                window.setScene(make_unique<TitleScene>());
            }
        }

        if (context.input.wasKeyPressed(GLFW_KEY_ESCAPE) ||
            context.input.wasKeyPressed(GLFW_KEY_ENTER) ||
            context.input.wasKeyPressed(GLFW_KEY_SPACE))
        {
            _isPaused = false;
        }
        return;
    }

    if (context.input.wasKeyPressed(GLFW_KEY_ESCAPE))
    {
        _state.cancelSelectedProduction();
    }

    const float viewWidth = static_cast<float>(window.getScreenWidth());
    const float maxCameraX = (std::max)(0.0f, kGameplayWorldWidth - viewWidth);

    float cameraVelocity = 0.0f;
    if (context.input.isKeyDown(GLFW_KEY_A) || context.input.isKeyDown(GLFW_KEY_LEFT))
    {
        cameraVelocity -= kCameraPanSpeed;
    }
    if (context.input.isKeyDown(GLFW_KEY_D) || context.input.isKeyDown(GLFW_KEY_RIGHT))
    {
        cameraVelocity += kCameraPanSpeed;
    }
    if (cursorScreenPosition.x <= kCameraEdgeThreshold)
    {
        cameraVelocity -= kCameraPanSpeed;
    }
    if (cursorScreenPosition.x >= viewWidth - kCameraEdgeThreshold)
    {
        cameraVelocity += kCameraPanSpeed;
    }

    _cameraX = glm::clamp(_cameraX + (cameraVelocity * static_cast<float>(deltaTime)), 0.0f, maxCameraX);
    const vec2 cursorWorldPosition = cursorScreenPosition + vec2(_cameraX, 0.0f);

    _state.update(context.input, deltaTime, cursorScreenPosition, cursorWorldPosition);
    context.currentSceneName = "GameplayScene | " + _state.getStatusText();

}

void GameplayScene::render(MainWindow& window, GameContext& context)
{
    (void)context;

    glClear(GL_COLOR_BUFFER_BIT);
    const float width = static_cast<float>(window.getScreenWidth());
    const float height = static_cast<float>(window.getScreenHeight());
    const mat4 worldProjection = ortho(_cameraX, _cameraX + width, 0.0f, height);
    _state.render(worldProjection, window.getOrthoProjectionMatrix(), window.getTimeDelta());

    if (_isPaused && _pauseShader != nullptr)
    {
        _pauseShader->use();
        _pauseShader->setMat4("projection_matrx", window.getOrthoProjectionMatrix());
        _pauseShader->setInt("tex", 0);

        if (_pauseBackdrop != nullptr)
        {
            _pauseBackdrop->SetTintColor(0.08f, 0.10f, 0.16f, 0.72f);
            _pauseBackdrop->SetDepth(0.18f);
            _pauseBackdrop->Draw();
        }
        if (_pauseWindow != nullptr)
        {
            _pauseWindow->SetTintColor(0.88f, 0.92f, 0.98f, 0.96f);
            _pauseWindow->SetDepth(0.19f);
            _pauseWindow->Draw();
        }
        if (_pauseHeader != nullptr)
        {
            _pauseHeader->SetTintColor(0.92f, 0.82f, 0.56f, 1.0f);
            _pauseHeader->SetDepth(0.20f);
            _pauseHeader->Draw();
        }
        if (_pauseResumeButton != nullptr)
        {
            const bool hovered = _hoveredPauseAction == PauseMenuAction::Resume;
            _pauseResumeButton->SetTintColor(hovered ? vec4(0.98f, 0.90f, 0.62f, 1.0f) : vec4(0.74f, 0.80f, 0.90f, 0.98f));
            _pauseResumeButton->SetDepth(0.20f);
            _pauseResumeButton->Draw();
        }
        if (_pauseTitleButton != nullptr)
        {
            const bool hovered = _hoveredPauseAction == PauseMenuAction::ReturnToTitle;
            _pauseTitleButton->SetTintColor(hovered ? vec4(0.98f, 0.82f, 0.62f, 1.0f) : vec4(0.74f, 0.80f, 0.90f, 0.98f));
            _pauseTitleButton->SetDepth(0.20f);
            _pauseTitleButton->Draw();
        }
    }

    if (_textReady)
    {
        const float sx = width / 1280.0f;
        const float sy = height / 720.0f;
        const float uiScale = (std::min)(sx, sy);
        const vec2 selectionTitlePos(156.0f * sx, 116.0f * sy);
        const vec2 selectionBodyPos(156.0f * sx, 90.0f * sy);
        const vec2 resourceTextPos(922.0f * sx, 668.0f * sy);
        const vec2 commandTextPos(932.0f * sx, 124.0f * sy);
        const vec2 statusTextPos(302.0f * sx, 116.0f * sy);
        const vec2 pausePanelPos((width * 0.5f) - (256.0f * uiScale), (height * 0.5f) - (160.0f * uiScale));
        const vec2 pauseHeaderPos(pausePanelPos + vec2(54.0f * uiScale, 232.0f * uiScale));
        const vec2 pauseResumePos(pausePanelPos + vec2(78.0f * uiScale, 150.0f * uiScale));
        const vec2 pauseTitlePos(pausePanelPos + vec2(78.0f * uiScale, 70.0f * uiScale));

        auto renderLine = [&](const string& text, const vec2& position, float scale, const vec3& color)
        {
            const wstring wide(text.begin(), text.end());
            _textEngine.renderText(wide, position.x, position.y, { scale, color });
        };

        renderLine(_state.buildSelectionTitle(), selectionTitlePos, 0.82f * uiScale, kHeaderColor);
        renderTextLines(_textEngine, _state.buildSelectionSummary(), selectionBodyPos, 0.54f * uiScale, kSelectionLineGap * uiScale, kBodyColor);
        renderTextLines(_textEngine, wrapTextToWidth(_textEngine, _state.buildTopBarText(), 0.50f * uiScale, kResourcePanelTextWidth * sx), resourceTextPos, 0.50f * uiScale, 18.0f * uiScale, kResourceColor);
        renderTextLines(_textEngine, _state.buildCommandHints(), commandTextPos, 0.48f * uiScale, kCommandLineGap * uiScale, kBodyColor);
        renderTextLines(_textEngine, wrapTextToWidth(_textEngine, _state.getStatusText(), 0.46f * uiScale, kInfoPanelTextWidth * sx), statusTextPos, 0.46f * uiScale, 18.0f * uiScale, kStatusColor);

        if (_state.hasCommandTooltip())
        {
            const vec2 tooltipPosition = _state.getCommandTooltipPosition();
            renderLine(_state.buildCommandTooltipTitle(), tooltipPosition + vec2(16.0f, 82.0f) * uiScale, 0.62f * uiScale, kHeaderColor);
            renderTextLines(_textEngine, _state.buildCommandTooltipLines(), tooltipPosition + vec2(16.0f, 54.0f) * uiScale, 0.40f * uiScale, 17.0f * uiScale, kBodyColor);
        }

        if (_state.isMatchOver())
        {
            const string resultText = _state.buildHudLines().back();
            renderLine(resultText, vec2(520.0f * sx, 392.0f * sy), 0.95f * uiScale, kResultColor);
        }

        if (_isPaused)
        {
            const vec3 pauseHeaderColor(1.0f, 1.0f, 1.0f);
            const vec3 pauseBodyColor(0.98f, 0.99f, 1.0f);
            const vec3 pauseHintColor(0.92f, 0.95f, 0.99f);

            renderLine("Pause Menu", pauseHeaderPos + vec2(92.0f, 18.0f) * uiScale, 0.92f * uiScale, pauseHeaderColor);
            renderLine("Resume Game", pauseResumePos + vec2(72.0f, 10.0f) * uiScale, 0.78f * uiScale, pauseBodyColor);
            renderLine("Return To Title", pauseTitlePos + vec2(48.0f, 10.0f) * uiScale, 0.78f * uiScale, pauseBodyColor);
            renderLine("F10 opens or closes this menu", pausePanelPos + vec2(82.0f, 64.0f) * uiScale, 0.48f * uiScale, pauseHintColor);
        }
    }
}

void GameplayScene::onResize(MainWindow& window, GameContext& context, int width, int height)
{
    (void)window;
    (void)context;

    if (width <= 0 || height <= 0)
    {
        return;
    }

    glViewport(0, 0, width, height);
    _textEngine.setProjection(width, height);
    _state.setViewportSize(width, height);

    const float uiScale = (std::min)(static_cast<float>(width) / 1280.0f, static_cast<float>(height) / 720.0f);
    const vec2 pausePanelPos((static_cast<float>(width) * 0.5f) - (256.0f * uiScale), (static_cast<float>(height) * 0.5f) - (160.0f * uiScale));
    const vec2 pausePanelSize(512.0f * uiScale, 320.0f * uiScale);
    const vec2 pauseHeaderPos(pausePanelPos + vec2(54.0f * uiScale, 232.0f * uiScale));
    const vec2 pauseHeaderSize(404.0f * uiScale, 66.0f * uiScale);
    const vec2 pauseResumePos(pausePanelPos + vec2(78.0f * uiScale, 150.0f * uiScale));
    const vec2 pauseTitlePos(pausePanelPos + vec2(78.0f * uiScale, 70.0f * uiScale));
    const vec2 pauseButtonSize(356.0f * uiScale, 58.0f * uiScale);

    if (_pauseBackdrop != nullptr)
    {
        _pauseBackdrop->SetPosition(vec2(0.0f, 0.0f));
        _pauseBackdrop->SetScale(vec2(static_cast<float>(width), static_cast<float>(height)));
    }
    if (_pauseWindow != nullptr)
    {
        _pauseWindow->SetPosition(pausePanelPos);
        _pauseWindow->SetScale(pausePanelSize);
    }
    if (_pauseHeader != nullptr)
    {
        _pauseHeader->SetPosition(pauseHeaderPos);
        _pauseHeader->SetScale(pauseHeaderSize);
    }
    if (_pauseResumeButton != nullptr)
    {
        _pauseResumeButton->SetPosition(pauseResumePos);
        _pauseResumeButton->SetScale(pauseButtonSize);
    }
    if (_pauseTitleButton != nullptr)
    {
        _pauseTitleButton->SetPosition(pauseTitlePos);
        _pauseTitleButton->SetScale(pauseButtonSize);
    }
}

GameplayScene::PauseMenuAction GameplayScene::hitTestPauseMenu(const vec2& cursorScreenPosition) const
{
    const auto inside = [&](const shared_ptr<Sprite>& sprite)
    {
        if (sprite == nullptr)
        {
            return false;
        }

        const vec2 pos = sprite->GetPosition();
        const vec2 size = sprite->GetScale();
        return cursorScreenPosition.x >= pos.x && cursorScreenPosition.x <= pos.x + size.x &&
            cursorScreenPosition.y >= pos.y && cursorScreenPosition.y <= pos.y + size.y;
    };

    if (inside(_pauseResumeButton))
    {
        return PauseMenuAction::Resume;
    }
    if (inside(_pauseTitleButton))
    {
        return PauseMenuAction::ReturnToTitle;
    }

    return PauseMenuAction::None;
}

void GameplayScene::initializePauseArt()
{
    if (_pauseShader == nullptr)
    {
        _pauseShader = make_shared<Shader>(kPauseVertexShader, kPauseFragmentShader);
    }

    if (_pauseBackdrop == nullptr)
    {
        _pauseBackdrop = make_shared<Sprite>(_pauseShader, "Assets/UI/hud_panel.png");
        _pauseWindow = make_shared<Sprite>(_pauseShader, "Assets/UI/production_panel.png");
        _pauseHeader = make_shared<Sprite>(_pauseShader, "Assets/UI/tooltip_panel.png");
        _pauseResumeButton = make_shared<Sprite>(_pauseShader, "Assets/UI/tooltip_panel.png");
        _pauseTitleButton = make_shared<Sprite>(_pauseShader, "Assets/UI/tooltip_panel.png");
    }
}
