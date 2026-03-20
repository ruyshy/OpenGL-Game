#include "pch.h"
#include "HowToPlayScene.h"

#include "MainWindow.h"
#include "Shader.h"
#include "Sprite.h"
#include "TitleScene.h"

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

void HowToPlayScene::onEnter(MainWindow& window, GameContext& context)
{
    context.currentSceneName = "HowToPlayScene";

    glClearColor(0.05f, 0.08f, 0.15f, 1.0f);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    _textReady = initializeTextEngine();
    initializeArt();
    onResize(window, context, window.getScreenWidth(), window.getScreenHeight());
}

void HowToPlayScene::onExit(MainWindow& window, GameContext& context)
{
    (void)window;
    (void)context;

    _textEngine.shutdown();
    _textReady = false;
    _menuShader.reset();
    _backdrop.reset();
    _contentPanel.reset();
    _headerPanel.reset();
}

void HowToPlayScene::update(MainWindow& window, GameContext& context, double deltaTime)
{
    (void)deltaTime;
    context.currentSceneName = "HowToPlayScene";

    if (context.input.wasKeyPressed(GLFW_KEY_ESCAPE) ||
        context.input.wasKeyPressed(GLFW_KEY_ENTER) ||
        context.input.wasKeyPressed(GLFW_KEY_SPACE) ||
        context.input.wasMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT) ||
        context.input.wasMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT))
    {
        window.setScene(make_unique<TitleScene>());
    }
}

void HowToPlayScene::render(MainWindow& window, GameContext& context)
{
    (void)context;
    glClear(GL_COLOR_BUFFER_BIT);

    if (_menuShader != nullptr)
    {
        _menuShader->use();
        _menuShader->setMat4("projection_matrx", window.getOrthoProjectionMatrix());
        _menuShader->setInt("tex", 0);

        if (_backdrop != nullptr)
        {
            _backdrop->SetDepth(-0.2f);
            _backdrop->Draw();
        }
        if (_contentPanel != nullptr)
        {
            _contentPanel->SetDepth(0.04f);
            _contentPanel->Draw();
        }
        if (_headerPanel != nullptr)
        {
            _headerPanel->SetDepth(0.06f);
            _headerPanel->Draw();
        }
    }

    if (!_textReady)
    {
        return;
    }

    const float width = static_cast<float>(window.getScreenWidth());
    const float height = static_cast<float>(window.getScreenHeight());
    const float uiScale = (std::min)(width / 1280.0f, height / 720.0f);
    const float centerX = width * 0.5f;
    const TextStyle titleStyle{ 1.04f * uiScale, vec3(0.98f, 0.94f, 0.78f) };
    const TextStyle bodyStyle{ 0.66f * uiScale, vec3(0.90f, 0.94f, 0.98f) };
    const TextStyle hintStyle{ 0.56f * uiScale, vec3(0.72f, 0.79f, 0.88f) };

    const auto renderCentered = [&](const wstring& text, float baselineY, const TextStyle& style)
    {
        const vec2 size = _textEngine.measureText(text, style.scale);
        _textEngine.render({ text, vec2(centerX - (size.x * 0.5f), baselineY), style });
    };

    renderCentered(L"\uAC8C\uC784\uBC29\uBC95", 578.0f * uiScale, titleStyle);
    renderCentered(L"\uC790\uC6D0\uC744 \uBAA8\uC544 \uC720\uB2DB\uC744 \uB9CC\uB4E4\uACE0 \uC801 HQ\uB97C \uD30C\uAD34\uD558\uC138\uC694.", 504.0f * uiScale, bodyStyle);
    renderCentered(L"1. W \uB294 \uBA54\uD0C8, E \uB294 \uC5D0\uB108\uC9C0 \uCC44\uC9D1", 432.0f * uiScale, bodyStyle);
    renderCentered(L"2. HQ \uC120\uD0DD \uD6C4 1 \uC77C\uAFBC, B \uBC30\uB7ED, F \uACF5\uC7A5 \uAC74\uC124", 374.0f * uiScale, bodyStyle);
    renderCentered(L"3. \uBC30\uB7ED\uC5D0\uC11C 2 \uC194\uC800, 3 \uB514\uD39C\uB354   \uACF5\uC7A5\uC5D0\uC11C 6 / 7", 316.0f * uiScale, bodyStyle);
    renderCentered(L"4. Esc \uB294 \uC120\uD0DD\uD55C \uC0DD\uC0B0 \uAC74\uBB3C\uC758 \uB9C8\uC9C0\uB9C9 \uB300\uAE30\uC5F4 \uCDE8\uC18C", 258.0f * uiScale, bodyStyle);
    renderCentered(L"5. F10 \uC740 \uC77C\uC2DC\uC815\uC9C0, \uC88C/\uC6B0\uD074\uB9AD\uC73C\uB85C \uC120\uD0DD\uACFC \uC774\uB3D9", 200.0f * uiScale, bodyStyle);
    renderCentered(L"6. \uC801 HQ\uB97C \uBA3C\uC800 \uBD80\uC218\uBA74 \uC2B9\uB9AC", 142.0f * uiScale, bodyStyle);
    renderCentered(L"Esc, Enter, \uC2A4\uD398\uC774\uC2A4, \uB9C8\uC6B0\uC2A4 \uD074\uB9AD\uC73C\uB85C \uBA54\uB274\uB85C \uB3CC\uC544\uAC11\uB2C8\uB2E4.", 74.0f * uiScale, hintStyle);
}

void HowToPlayScene::onResize(MainWindow& window, GameContext& context, int width, int height)
{
    (void)context;

    if (width <= 0 || height <= 0)
    {
        return;
    }

    glViewport(0, 0, width, height);
    _textEngine.setProjection(width, height);
    const float uiScale = (std::min)(static_cast<float>(width) / 1280.0f, static_cast<float>(height) / 720.0f);

    if (_backdrop != nullptr)
    {
        _backdrop->SetPosition(vec2(20.0f * uiScale, 16.0f * uiScale));
        _backdrop->SetScale(vec2(static_cast<float>(width) - (40.0f * uiScale), static_cast<float>(height) - (32.0f * uiScale)));
    }

    if (_contentPanel != nullptr)
    {
        _contentPanel->SetPosition(vec2(150.0f * uiScale, 58.0f * uiScale));
        _contentPanel->SetScale(vec2(static_cast<float>(width) - (300.0f * uiScale), static_cast<float>(height) - (140.0f * uiScale)));
    }

    if (_headerPanel != nullptr)
    {
        _headerPanel->SetPosition(vec2((static_cast<float>(width) * 0.5f) - (240.0f * uiScale), static_cast<float>(height) - (176.0f * uiScale)));
        _headerPanel->SetScale(vec2(480.0f * uiScale, 92.0f * uiScale));
    }
}

bool HowToPlayScene::initializeTextEngine()
{
    const string fontPath = findFontPath();
    if (fontPath.empty())
    {
        cerr << "No usable system font was found for FreeType text rendering." << endl;
        return false;
    }

    return _textEngine.initialize(fontPath, 52);
}

void HowToPlayScene::initializeArt()
{
    if (_menuShader == nullptr)
    {
        _menuShader = make_shared<Shader>(kSpriteVertexShader, kSpriteFragmentShader);
    }

    if (_backdrop == nullptr)
    {
        _backdrop = make_shared<Sprite>(_menuShader, "Assets/UI/hud_panel.png");
        _contentPanel = make_shared<Sprite>(_menuShader, "Assets/UI/production_panel.png");
        _headerPanel = make_shared<Sprite>(_menuShader, "Assets/UI/tooltip_panel.png");
    }
}
