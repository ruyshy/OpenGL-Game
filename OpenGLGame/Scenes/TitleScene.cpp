#include "pch.h"
#include "TitleScene.h"

#include "GameplayScene.h"
#include "HowToPlayScene.h"
#include "MainWindow.h"
#include "Shader.h"
#include "Sprite.h"

namespace
{
    const string kSpriteVertexShader = R"(#version 330 core
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

    const string kSpriteFragmentShader = R"(#version 330 core
in vec4 vertexColor;
in vec2 texCoord;

out vec4 FragColor;

uniform sampler2D tex;

void main()
{
    FragColor = texture(tex, texCoord) * vertexColor;
}
)";

    string findFontPath()
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
}

void TitleScene::onEnter(MainWindow& window, GameContext& context)
{
    context.currentSceneName = "TitleScene";

    glClearColor(0.08f, 0.10f, 0.18f, 1.0f);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    _selectedButton = TitleMenuButton::SinglePlayer;
    _menuStatus = L"\uBA54\uB274\uB97C \uC120\uD0DD\uD558\uC138\uC694.";

    _textReady = initializeTextEngine();
    initializeMenuArt();
    updateButtonLayout(window);
    onResize(window, context, window.getScreenWidth(), window.getScreenHeight());
}

void TitleScene::onExit(MainWindow& window, GameContext& context)
{
    (void)window;
    (void)context;

    _textEngine.shutdown();
    _textReady = false;
    _menuShader.reset();
    _menuBackdrop.reset();
    _menuTitlePanel.reset();
    _singlePlayerButton.reset();
    _multiPlayerButton.reset();
    _howToPlayButton.reset();
}

void TitleScene::update(MainWindow& window, GameContext& context, double deltaTime)
{
    (void)deltaTime;

    bool hit = false;
    const vec2 cursorPosition = vec2(window.getOpenGLCursorPosition());
    const TitleMenuButton hoveredButton = hitTestButton(cursorPosition, hit);
    if (hit)
    {
        _selectedButton = hoveredButton;
    }

    if (context.input.wasKeyPressed(GLFW_KEY_UP) || context.input.wasKeyPressed(GLFW_KEY_W))
    {
        if (_selectedButton == TitleMenuButton::SinglePlayer) _selectedButton = TitleMenuButton::HowToPlay;
        else if (_selectedButton == TitleMenuButton::MultiPlayer) _selectedButton = TitleMenuButton::SinglePlayer;
        else _selectedButton = TitleMenuButton::MultiPlayer;
    }

    if (context.input.wasKeyPressed(GLFW_KEY_DOWN) || context.input.wasKeyPressed(GLFW_KEY_S))
    {
        if (_selectedButton == TitleMenuButton::SinglePlayer) _selectedButton = TitleMenuButton::MultiPlayer;
        else if (_selectedButton == TitleMenuButton::MultiPlayer) _selectedButton = TitleMenuButton::HowToPlay;
        else _selectedButton = TitleMenuButton::SinglePlayer;
    }

    if (context.input.wasKeyPressed(GLFW_KEY_ENTER) ||
        context.input.wasKeyPressed(GLFW_KEY_SPACE) ||
        context.input.wasMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT))
    {
        activateButton(window);
    }
}

void TitleScene::render(MainWindow& window, GameContext& context)
{
    (void)context;

    glClear(GL_COLOR_BUFFER_BIT);

    if (!_textReady)
    {
        return;
    }

    const float width = static_cast<float>(window.getScreenWidth());
    const float height = static_cast<float>(window.getScreenHeight());
    const float uiScale = (std::min)(width / 1280.0f, height / 720.0f);
    const float centerX = width * 0.5f;
    const float titleBaseline = height * 0.77f;
    const float subtitleBaseline = titleBaseline - 56.0f;
    const float buttonCenterY = height * 0.52f;
    const float buttonSpacing = 98.0f * uiScale;

    if (_menuShader != nullptr)
    {
        _menuShader->use();
        _menuShader->setMat4("projection_matrx", window.getOrthoProjectionMatrix());
        _menuShader->setInt("tex", 0);

        if (_menuBackdrop != nullptr)
        {
            _menuBackdrop->SetDepth(-0.2f);
            _menuBackdrop->Draw();
        }
        if (_menuTitlePanel != nullptr)
        {
            _menuTitlePanel->SetDepth(0.05f);
            _menuTitlePanel->Draw();
        }

        const array<pair<shared_ptr<Sprite>, TitleMenuButton>, 3> buttons =
        {{
            { _singlePlayerButton, TitleMenuButton::SinglePlayer },
            { _multiPlayerButton, TitleMenuButton::MultiPlayer },
            { _howToPlayButton, TitleMenuButton::HowToPlay }
        }};

        for (const auto& [buttonSprite, buttonId] : buttons)
        {
            if (buttonSprite == nullptr)
            {
                continue;
            }

            const bool isSelected = _selectedButton == buttonId;
            buttonSprite->SetScale(isSelected ? vec2(388.0f * uiScale, 84.0f * uiScale) : vec2(372.0f * uiScale, 76.0f * uiScale));
            buttonSprite->SetDepth(isSelected ? 0.12f : 0.10f);
            buttonSprite->Draw();
        }
    }

    const wstring title = L"Project Sandforge";
    const wstring subtitle = L"Sandstorm Frontline Prototype";
    const wstring singlePlayer = L"\uC2F1\uAE00\uD50C\uB808\uC774";
    const wstring multiPlayer = L"\uBA40\uD2F0\uD50C\uB808\uC774";
    const wstring howToPlay = L"\uAC8C\uC784\uBC29\uBC95";
    const wstring footer = L"\uBC29\uD5A5\uD0A4 \uB610\uB294 \uB9C8\uC6B0\uC2A4\uB85C \uC120\uD0DD \uD6C4 Enter / \uC88C\uD074\uB9AD";

    const TextStyle titleStyle{ 1.28f * uiScale, vec3(0.95f, 0.96f, 0.98f) };
    const TextStyle subtitleStyle{ 0.72f * uiScale, vec3(0.83f, 0.89f, 0.98f) };
    const TextStyle buttonNormalStyle{ 0.82f * uiScale, vec3(0.84f, 0.89f, 0.96f) };
    const TextStyle buttonActiveStyle{ 0.92f * uiScale, vec3(1.0f, 0.95f, 0.74f) };
    const TextStyle footerStyle{ 0.56f * uiScale, vec3(0.65f, 0.72f, 0.82f) };
    const TextStyle statusStyle{ 0.58f * uiScale, vec3(0.82f, 0.88f, 0.96f) };

    const auto renderCentered = [&](const wstring& text, float baselineY, const TextStyle& style)
    {
        const vec2 size = _textEngine.measureText(text, style.scale);
        _textEngine.render({ text, vec2(centerX - (size.x * 0.5f), baselineY), style });
    };

    renderCentered(title, titleBaseline, titleStyle);
    renderCentered(subtitle, subtitleBaseline, subtitleStyle);
    renderCentered(singlePlayer, buttonCenterY, _selectedButton == TitleMenuButton::SinglePlayer ? buttonActiveStyle : buttonNormalStyle);
    renderCentered(multiPlayer, buttonCenterY - buttonSpacing, _selectedButton == TitleMenuButton::MultiPlayer ? buttonActiveStyle : buttonNormalStyle);
    renderCentered(howToPlay, buttonCenterY - (buttonSpacing * 2.0f), _selectedButton == TitleMenuButton::HowToPlay ? buttonActiveStyle : buttonNormalStyle);
    renderCentered(footer, 86.0f, footerStyle);
    renderCentered(_menuStatus, 52.0f, statusStyle);

}

void TitleScene::onResize(MainWindow& window, GameContext& context, int width, int height)
{
    (void)window;
    (void)context;

    if (width <= 0 || height <= 0)
    {
        return;
    }

    glViewport(0, 0, width, height);
    _textEngine.setProjection(width, height);
    updateButtonLayout(window);
}

bool TitleScene::initializeTextEngine()
{
    const string fontPath = findFontPath();
    if (fontPath.empty())
    {
        cerr << "No usable system font was found for FreeType text rendering." << endl;
        return false;
    }

    return _textEngine.initialize(fontPath, 48);
}

void TitleScene::initializeMenuArt()
{
    if (_menuShader == nullptr)
    {
        _menuShader = make_shared<Shader>(kSpriteVertexShader, kSpriteFragmentShader);
    }

    if (_menuBackdrop == nullptr)
    {
        _menuBackdrop = make_shared<Sprite>(_menuShader, "Assets/UI/hud_panel.png");
        _menuTitlePanel = make_shared<Sprite>(_menuShader, "Assets/UI/tooltip_panel.png");
        _singlePlayerButton = make_shared<Sprite>(_menuShader, "Assets/UI/production_panel.png");
        _multiPlayerButton = make_shared<Sprite>(_menuShader, "Assets/UI/production_panel.png");
        _howToPlayButton = make_shared<Sprite>(_menuShader, "Assets/UI/production_panel.png");
    }
}

void TitleScene::updateButtonLayout(MainWindow& window)
{
    if (_menuBackdrop == nullptr)
    {
        return;
    }

    const float width = static_cast<float>(window.getScreenWidth());
    const float height = static_cast<float>(window.getScreenHeight());
    const float uiScale = (std::min)(width / 1280.0f, height / 720.0f);
    const float centerX = width * 0.5f;
    const float buttonWidth = 372.0f * uiScale;
    const float buttonHeight = 76.0f * uiScale;
    const float buttonSpacing = 98.0f * uiScale;
    const float firstButtonY = height * 0.5f - (34.0f * uiScale);
    const float buttonX = centerX - (buttonWidth * 0.5f);
    const float backdropWidth = width - (40.0f * uiScale);
    const float backdropHeight = height - (32.0f * uiScale);

    _menuBackdrop->SetPosition(vec2((width - backdropWidth) * 0.5f, (height - backdropHeight) * 0.5f));
    _menuBackdrop->SetScale(vec2(backdropWidth, backdropHeight));
    _menuTitlePanel->SetPosition(vec2(centerX - (280.0f * uiScale), height * 0.5f + (150.0f * uiScale)));
    _menuTitlePanel->SetScale(vec2(560.0f * uiScale, 130.0f * uiScale));

    _singlePlayerButton->SetPosition(vec2(buttonX, firstButtonY));
    _singlePlayerButton->SetScale(vec2(buttonWidth, buttonHeight));
    _multiPlayerButton->SetPosition(vec2(buttonX, firstButtonY - buttonSpacing));
    _multiPlayerButton->SetScale(vec2(buttonWidth, buttonHeight));
    _howToPlayButton->SetPosition(vec2(buttonX, firstButtonY - (buttonSpacing * 2.0f)));
    _howToPlayButton->SetScale(vec2(buttonWidth, buttonHeight));
}

void TitleScene::activateButton(MainWindow& window)
{
    if (_selectedButton == TitleMenuButton::SinglePlayer)
    {
        _menuStatus = L"\uC2F1\uAE00\uD50C\uB808\uC774\uB97C \uC2DC\uC791\uD569\uB2C8\uB2E4.";
        window.setScene(make_unique<GameplayScene>());
        return;
    }

    if (_selectedButton == TitleMenuButton::MultiPlayer)
    {
        _menuStatus = L"\uBA40\uD2F0\uD50C\uB808\uC774\uB294 \uC544\uC9C1 \uC900\uBE44 \uC911\uC785\uB2C8\uB2E4.";
        return;
    }

    _menuStatus = L"\uAC8C\uC784\uBC29\uBC95 \uD654\uBA74\uC73C\uB85C \uC774\uB3D9\uD569\uB2C8\uB2E4.";
    window.setScene(make_unique<HowToPlayScene>());
}

TitleMenuButton TitleScene::hitTestButton(const vec2& cursorPosition, bool& hit) const
{
    const array<pair<shared_ptr<Sprite>, TitleMenuButton>, 3> buttons =
    {{
        { _singlePlayerButton, TitleMenuButton::SinglePlayer },
        { _multiPlayerButton, TitleMenuButton::MultiPlayer },
        { _howToPlayButton, TitleMenuButton::HowToPlay }
    }};

    for (const auto& [sprite, buttonId] : buttons)
    {
        if (sprite == nullptr)
        {
            continue;
        }

        const vec2 position = sprite->GetPosition();
        const vec2 size = sprite->GetScale();
        if (cursorPosition.x >= position.x && cursorPosition.x <= position.x + size.x &&
            cursorPosition.y >= position.y && cursorPosition.y <= position.y + size.y)
        {
            hit = true;
            return buttonId;
        }
    }

    hit = false;
    return TitleMenuButton::SinglePlayer;
}
