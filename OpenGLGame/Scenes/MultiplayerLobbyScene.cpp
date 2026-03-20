#include "pch.h"
#include "MultiplayerLobbyScene.h"

#include "GameContext.h"
#include "MainWindow.h"
#include "GameplayScene.h"
#include "Network/SandforgeMultiplayerSession.h"
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

    void appendPressedDigit(const InputState& input, int keyCode, int keypadKeyCode, char digit, string& target, size_t maxLength)
    {
        if ((input.wasKeyPressed(keyCode) || input.wasKeyPressed(keypadKeyCode)) && target.size() < maxLength)
        {
            target.push_back(digit);
        }
    }

    void appendPressedLetter(const InputState& input, int keyCode, char upperCaseLetter, string& target, size_t maxLength)
    {
        if (input.wasKeyPressed(keyCode) && target.size() < maxLength)
        {
            target.push_back(upperCaseLetter);
        }
    }

    vector<wstring> wrapLobbyText(TextEngine& textEngine, const wstring& text, float scale, float maxWidth)
    {
        if (text.empty())
        {
            return {};
        }

        wistringstream stream(text);
        wstring word;
        wstring currentLine;
        vector<wstring> lines;

        while (stream >> word)
        {
            const wstring candidate = currentLine.empty() ? word : currentLine + L" " + word;
            if (!currentLine.empty() && textEngine.measureText(candidate, scale).x > maxWidth)
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

    bool isHostOnlyLobbyButton(MultiplayerLobbyButton button)
    {
        return button == MultiplayerLobbyButton::ResourcePresetField ||
            button == MultiplayerLobbyButton::WorkerPresetField ||
            button == MultiplayerLobbyButton::StartMatch;
    }
}

void MultiplayerLobbyScene::onEnter(MainWindow& window, GameContext& context)
{
    context.currentSceneName = "MultiplayerLobbyScene";
    if (!context.multiplayerSession)
    {
        context.multiplayerSession = make_shared<SandforgeMultiplayerSession>();
    }
    context.multiplayerSession->setLocalPlayerName(_localNickname);

    glClearColor(0.05f, 0.08f, 0.14f, 1.0f);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    _selectedButton = MultiplayerLobbyButton::Host;
    _textReady = initializeTextEngine();
    initializeArt();
    updateLayout(window);
}

void MultiplayerLobbyScene::onExit(MainWindow& window, GameContext& context)
{
    (void)window;
    (void)context;
    _textEngine.shutdown();
    _textReady = false;
    _menuShader.reset();
    _backdrop.reset();
    _panel.reset();
    _hostButton.reset();
    _nicknameField.reset();
    _addressField.reset();
    _portField.reset();
    _resourceField.reset();
    _workerField.reset();
    _startButton.reset();
    _joinButton.reset();
    _readyButton.reset();
    _backButton.reset();
}

void MultiplayerLobbyScene::update(MainWindow& window, GameContext& context, double deltaTime)
{
    (void)deltaTime;
    if (context.multiplayerSession)
    {
        context.multiplayerSession->update();
        const SandforgeLobbyState& lobbyState = context.multiplayerSession->getLobbyState();
        _resourcePreset = lobbyState.resourcePreset;
        _workerPreset = lobbyState.workerPreset;
        if (context.multiplayerSession->consumeMatchStart())
        {
            window.setScene(make_unique<GameplayScene>());
            return;
        }
    }

    if (_editingNickname)
    {
        handleNicknameEditing(context);
        return;
    }
    if (_editingAddress)
    {
        handleAddressEditing(context);
        return;
    }
    if (_editingPort)
    {
        handlePortEditing(context);
        return;
    }

    bool hit = false;
    const vec2 cursorPosition = vec2(window.getOpenGLCursorPosition());
    const MultiplayerLobbyButton hoveredButton = hitTestButton(cursorPosition, hit);
    if (hit)
    {
        _selectedButton = hoveredButton;
    }

    if (context.input.wasKeyPressed(GLFW_KEY_UP) || context.input.wasKeyPressed(GLFW_KEY_W))
    {
        if (_selectedButton == MultiplayerLobbyButton::Host) _selectedButton = MultiplayerLobbyButton::Back;
        else _selectedButton = static_cast<MultiplayerLobbyButton>(static_cast<int>(_selectedButton) - 1);
    }
    if (context.input.wasKeyPressed(GLFW_KEY_DOWN) || context.input.wasKeyPressed(GLFW_KEY_S))
    {
        if (_selectedButton == MultiplayerLobbyButton::Back) _selectedButton = MultiplayerLobbyButton::Host;
        else _selectedButton = static_cast<MultiplayerLobbyButton>(static_cast<int>(_selectedButton) + 1);
    }

    if (context.input.wasKeyPressed(GLFW_KEY_ENTER) ||
        context.input.wasKeyPressed(GLFW_KEY_SPACE) ||
        context.input.wasMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT))
    {
        activateButton(window, context);
    }

    if (context.input.wasKeyPressed(GLFW_KEY_ESCAPE))
    {
        if (context.multiplayerSession)
        {
            context.multiplayerSession->disconnect();
        }
        window.setScene(make_unique<TitleScene>());
    }
}

void MultiplayerLobbyScene::render(MainWindow& window, GameContext& context)
{
    glClear(GL_COLOR_BUFFER_BIT);
    if (!_textReady)
    {
        return;
    }

    const float width = static_cast<float>(window.getScreenWidth());
    const float height = static_cast<float>(window.getScreenHeight());
    const float uiScale = (std::min)(width / 1280.0f, height / 720.0f);
    const float centerX = width * 0.5f;
    const float titleY = height * 0.80f;
    const float firstButtonY = height * 0.54f;
    const float buttonGap = 40.0f * uiScale;
    const SandforgeLobbyState* lobbyState = context.multiplayerSession ? &context.multiplayerSession->getLobbyState() : nullptr;
    const bool hostControlsEnabled = lobbyState != nullptr && lobbyState->mode == SandforgeMultiplayerMode::Host;
    const bool canStartMatch = lobbyState != nullptr &&
        lobbyState->mode == SandforgeMultiplayerMode::Host &&
        lobbyState->remoteConnected &&
        lobbyState->localReady &&
        lobbyState->remoteReady;

    if (_menuShader != nullptr)
    {
        _menuShader->use();
        _menuShader->setMat4("projection_matrx", window.getOrthoProjectionMatrix());
        _menuShader->setInt("tex", 0);

        if (_backdrop != nullptr)
        {
            _backdrop->SetDepth(-0.1f);
            _backdrop->Draw();
        }
        if (_panel != nullptr)
        {
            _panel->SetDepth(0.05f);
            _panel->Draw();
        }

        const array<pair<shared_ptr<Sprite>, MultiplayerLobbyButton>, 10> buttons =
        {{
            { _hostButton, MultiplayerLobbyButton::Host },
            { _nicknameField, MultiplayerLobbyButton::NicknameField },
            { _addressField, MultiplayerLobbyButton::AddressField },
            { _portField, MultiplayerLobbyButton::PortField },
            { _resourceField, MultiplayerLobbyButton::ResourcePresetField },
            { _workerField, MultiplayerLobbyButton::WorkerPresetField },
            { _startButton, MultiplayerLobbyButton::StartMatch },
            { _joinButton, MultiplayerLobbyButton::Join },
            { _readyButton, MultiplayerLobbyButton::Ready },
            { _backButton, MultiplayerLobbyButton::Back }
        }};

        for (const auto& [sprite, buttonId] : buttons)
        {
            if (sprite == nullptr) continue;
            const bool selected = _selectedButton == buttonId;
            const bool disabled = isHostOnlyLobbyButton(buttonId) && !hostControlsEnabled;
            vec4 tint(1.0f, 1.0f, 1.0f, 1.0f);
            if (buttonId == MultiplayerLobbyButton::Ready && lobbyState != nullptr && lobbyState->localReady)
            {
                tint = vec4(0.72f, 0.94f, 0.74f, 1.0f);
            }
            else if (buttonId == MultiplayerLobbyButton::StartMatch && canStartMatch)
            {
                tint = vec4(0.96f, 0.88f, 0.60f, 1.0f);
            }
            else if (buttonId == MultiplayerLobbyButton::Join && lobbyState != nullptr && lobbyState->remoteConnected)
            {
                tint = vec4(0.70f, 0.84f, 0.98f, 1.0f);
            }

            sprite->SetScale(selected ? vec2(382.0f * uiScale, 78.0f * uiScale) : vec2(364.0f * uiScale, 72.0f * uiScale));
            if (disabled)
            {
                tint = vec4(0.46f, 0.50f, 0.58f, 0.82f);
            }
            sprite->SetTintColor(tint);
            sprite->SetDepth(selected ? 0.12f : 0.10f);
            sprite->Draw();
        }
    }

    const auto renderCentered = [&](const wstring& text, float baselineY, const TextStyle& style)
    {
        const vec2 size = _textEngine.measureText(text, style.scale);
        _textEngine.render({ text, vec2(centerX - (size.x * 0.5f), baselineY), style });
    };

    const TextStyle titleStyle{ 1.12f * uiScale, vec3(0.95f, 0.96f, 0.98f) };
    const TextStyle bodyStyle{ 0.72f * uiScale, vec3(0.84f, 0.90f, 0.98f) };
    const TextStyle activeStyle{ 0.82f * uiScale, vec3(1.0f, 0.94f, 0.72f) };
    const TextStyle statusStyle{ 0.38f * uiScale, vec3(0.82f, 0.88f, 0.96f) };
    const TextStyle fieldStyle{ 0.52f * uiScale, vec3(0.82f, 0.88f, 0.96f) };
    const TextStyle activeFieldStyle{ 0.52f * uiScale, vec3(1.0f, 0.94f, 0.72f) };
    const TextStyle disabledStyle{ 0.52f * uiScale, vec3(0.50f, 0.56f, 0.64f) };
    const TextStyle readyStyle{ 0.44f * uiScale, vec3(0.74f, 0.96f, 0.78f) };
    const TextStyle waitStyle{ 0.44f * uiScale, vec3(0.96f, 0.82f, 0.62f) };
    const TextStyle offlineStyle{ 0.44f * uiScale, vec3(0.74f, 0.76f, 0.82f) };

    const auto hostAwareFieldStyle = [&](MultiplayerLobbyButton button, bool active) -> TextStyle
    {
        if (isHostOnlyLobbyButton(button) && !hostControlsEnabled)
        {
            return disabledStyle;
        }
        return active ? activeFieldStyle : fieldStyle;
    };

    const auto hostAwareActionStyle = [&](MultiplayerLobbyButton button, bool active) -> TextStyle
    {
        if (isHostOnlyLobbyButton(button) && !hostControlsEnabled)
        {
            return disabledStyle;
        }
        return active ? activeStyle : bodyStyle;
    };

    renderCentered(L"Multiplayer Lobby", titleY, titleStyle);
    renderCentered(L"Host or join a LAN-ready prototype session", titleY - (44.0f * uiScale), bodyStyle);
    if (lobbyState != nullptr)
    {
        const string localLine = string("You: ") +
            (lobbyState->localPlayerName.empty() ? "Player" : lobbyState->localPlayerName) +
            (lobbyState->localReady ? " | READY" : " | NOT READY");
        const string remoteLine = string("Peer: ") +
            (lobbyState->remotePlayerName.empty() ? "Remote" : lobbyState->remotePlayerName) +
            (lobbyState->remoteConnected ? (lobbyState->remoteReady ? " | CONNECTED | READY" : " | CONNECTED | WAITING") : " | NOT CONNECTED");
        const string startLine = canStartMatch ? "Match can start now" : "Waiting for host setup and both players ready";

        renderCentered(wstring(localLine.begin(), localLine.end()), titleY - (72.0f * uiScale), lobbyState->localReady ? readyStyle : waitStyle);
        renderCentered(wstring(remoteLine.begin(), remoteLine.end()), titleY - (94.0f * uiScale), lobbyState->remoteConnected ? (lobbyState->remoteReady ? readyStyle : waitStyle) : offlineStyle);
        renderCentered(wstring(startLine.begin(), startLine.end()), titleY - (116.0f * uiScale), canStartMatch ? readyStyle : bodyStyle);
    }
    renderCentered(L"Host Local Session", firstButtonY, _selectedButton == MultiplayerLobbyButton::Host ? activeStyle : bodyStyle);
    renderCentered(buildNicknameText(), firstButtonY - buttonGap, (_selectedButton == MultiplayerLobbyButton::NicknameField || _editingNickname) ? activeFieldStyle : fieldStyle);
    renderCentered(buildAddressText(), firstButtonY - (buttonGap * 2.0f), (_selectedButton == MultiplayerLobbyButton::AddressField || _editingAddress) ? activeFieldStyle : fieldStyle);
    renderCentered(buildPortText(), firstButtonY - (buttonGap * 3.0f), (_selectedButton == MultiplayerLobbyButton::PortField || _editingPort) ? activeFieldStyle : fieldStyle);
    renderCentered(buildResourcePresetText(), firstButtonY - (buttonGap * 4.0f), hostAwareFieldStyle(MultiplayerLobbyButton::ResourcePresetField, _selectedButton == MultiplayerLobbyButton::ResourcePresetField));
    renderCentered(buildWorkerPresetText(), firstButtonY - (buttonGap * 5.0f), hostAwareFieldStyle(MultiplayerLobbyButton::WorkerPresetField, _selectedButton == MultiplayerLobbyButton::WorkerPresetField));
    renderCentered(L"Start Match", firstButtonY - (buttonGap * 6.0f), hostAwareActionStyle(MultiplayerLobbyButton::StartMatch, _selectedButton == MultiplayerLobbyButton::StartMatch));
    renderCentered(L"Join Session", firstButtonY - (buttonGap * 7.0f), _selectedButton == MultiplayerLobbyButton::Join ? activeStyle : bodyStyle);
    renderCentered(L"Toggle Ready", firstButtonY - (buttonGap * 8.0f), _selectedButton == MultiplayerLobbyButton::Ready ? activeStyle : bodyStyle);
    renderCentered(L"Back To Title", firstButtonY - (buttonGap * 9.0f), _selectedButton == MultiplayerLobbyButton::Back ? activeStyle : bodyStyle);
    const vector<wstring> statusLines = wrapLobbyText(_textEngine, buildStatusText(context), statusStyle.scale, width * 0.86f);
    float statusY = 34.0f * uiScale;
    for (const wstring& line : statusLines)
    {
        renderCentered(line, statusY, statusStyle);
        statusY += 18.0f * uiScale;
    }
    if (!hostControlsEnabled && lobbyState != nullptr && lobbyState->mode == SandforgeMultiplayerMode::Client)
    {
        renderCentered(L"Host controls Resources / Workers / Start Match", 16.0f * uiScale, disabledStyle);
    }
}

void MultiplayerLobbyScene::onResize(MainWindow& window, GameContext& context, int width, int height)
{
    (void)window;
    (void)context;
    if (width <= 0 || height <= 0)
    {
        return;
    }

    glViewport(0, 0, width, height);
    _textEngine.setProjection(width, height);
    updateLayout(window);
}

bool MultiplayerLobbyScene::initializeTextEngine()
{
    const string fontPath = findFontPath();
    return !fontPath.empty() && _textEngine.initialize(fontPath, 42);
}

void MultiplayerLobbyScene::initializeArt()
{
    if (_menuShader == nullptr)
    {
        _menuShader = make_shared<Shader>(kSpriteVertexShader, kSpriteFragmentShader);
    }

    if (_backdrop == nullptr)
    {
        _backdrop = make_shared<Sprite>(_menuShader, "Assets/UI/hud_panel.png");
        _panel = make_shared<Sprite>(_menuShader, "Assets/UI/tooltip_panel.png");
        _hostButton = make_shared<Sprite>(_menuShader, "Assets/UI/production_panel.png");
        _nicknameField = make_shared<Sprite>(_menuShader, "Assets/UI/tooltip_panel.png");
        _addressField = make_shared<Sprite>(_menuShader, "Assets/UI/tooltip_panel.png");
        _portField = make_shared<Sprite>(_menuShader, "Assets/UI/tooltip_panel.png");
        _resourceField = make_shared<Sprite>(_menuShader, "Assets/UI/tooltip_panel.png");
        _workerField = make_shared<Sprite>(_menuShader, "Assets/UI/tooltip_panel.png");
        _startButton = make_shared<Sprite>(_menuShader, "Assets/UI/production_panel.png");
        _joinButton = make_shared<Sprite>(_menuShader, "Assets/UI/production_panel.png");
        _readyButton = make_shared<Sprite>(_menuShader, "Assets/UI/production_panel.png");
        _backButton = make_shared<Sprite>(_menuShader, "Assets/UI/production_panel.png");
    }
}

void MultiplayerLobbyScene::updateLayout(MainWindow& window)
{
    const float width = static_cast<float>(window.getScreenWidth());
    const float height = static_cast<float>(window.getScreenHeight());
    const float uiScale = (std::min)(width / 1280.0f, height / 720.0f);
    const float buttonWidth = 364.0f * uiScale;
    const float buttonHeight = 72.0f * uiScale;
    const float centerX = (width - buttonWidth) * 0.5f;
    const float firstButtonY = (height * 0.54f) - (buttonHeight * 0.5f);
    const float buttonGap = 40.0f * uiScale;

    if (_backdrop != nullptr)
    {
        _backdrop->SetPosition(vec2(0.0f, 0.0f));
        _backdrop->SetScale(vec2(width, height));
    }
    if (_panel != nullptr)
    {
        _panel->SetPosition(vec2(width * 0.18f, height * 0.07f));
        _panel->SetScale(vec2(width * 0.64f, height * 0.82f));
    }

    _hostButton->SetPosition(vec2(centerX, firstButtonY));
    _hostButton->SetScale(vec2(buttonWidth, buttonHeight));
    _nicknameField->SetPosition(vec2(centerX, firstButtonY - buttonGap));
    _nicknameField->SetScale(vec2(buttonWidth, buttonHeight));
    _addressField->SetPosition(vec2(centerX, firstButtonY - (buttonGap * 2.0f)));
    _addressField->SetScale(vec2(buttonWidth, buttonHeight));
    _portField->SetPosition(vec2(centerX, firstButtonY - (buttonGap * 3.0f)));
    _portField->SetScale(vec2(buttonWidth, buttonHeight));
    _resourceField->SetPosition(vec2(centerX, firstButtonY - (buttonGap * 4.0f)));
    _resourceField->SetScale(vec2(buttonWidth, buttonHeight));
    _workerField->SetPosition(vec2(centerX, firstButtonY - (buttonGap * 5.0f)));
    _workerField->SetScale(vec2(buttonWidth, buttonHeight));
    _startButton->SetPosition(vec2(centerX, firstButtonY - (buttonGap * 6.0f)));
    _startButton->SetScale(vec2(buttonWidth, buttonHeight));
    _joinButton->SetPosition(vec2(centerX, firstButtonY - (buttonGap * 7.0f)));
    _joinButton->SetScale(vec2(buttonWidth, buttonHeight));
    _readyButton->SetPosition(vec2(centerX, firstButtonY - (buttonGap * 8.0f)));
    _readyButton->SetScale(vec2(buttonWidth, buttonHeight));
    _backButton->SetPosition(vec2(centerX, firstButtonY - (buttonGap * 9.0f)));
    _backButton->SetScale(vec2(buttonWidth, buttonHeight));
}

void MultiplayerLobbyScene::activateButton(MainWindow& window, GameContext& context)
{
    if (!context.multiplayerSession)
    {
        context.multiplayerSession = make_shared<SandforgeMultiplayerSession>();
    }

    if (_selectedButton == MultiplayerLobbyButton::Host)
    {
        context.multiplayerSession->setLocalPlayerName(_localNickname);
        _editingNickname = false;
        _editingAddress = false;
        _editingPort = false;
        context.multiplayerSession->host(_sessionPort);
        return;
    }
    if (_selectedButton == MultiplayerLobbyButton::NicknameField)
    {
        _editingAddress = false;
        _editingPort = false;
        _editingNickname = true;
        return;
    }
    if (_selectedButton == MultiplayerLobbyButton::AddressField)
    {
        _editingNickname = false;
        _editingPort = false;
        _editingAddress = true;
        return;
    }
    if (_selectedButton == MultiplayerLobbyButton::PortField)
    {
        _editingNickname = false;
        _editingAddress = false;
        _editingPort = true;
        return;
    }
    if (_selectedButton == MultiplayerLobbyButton::ResourcePresetField)
    {
        if (context.multiplayerSession && context.multiplayerSession->getLobbyState().mode == SandforgeMultiplayerMode::Host)
        {
            _resourcePreset = _resourcePreset == SandforgeStartResourcePreset::Standard ? SandforgeStartResourcePreset::Rich : SandforgeStartResourcePreset::Standard;
            context.multiplayerSession->setLobbyMatchSettings(_resourcePreset, _workerPreset);
        }
        return;
    }
    if (_selectedButton == MultiplayerLobbyButton::WorkerPresetField)
    {
        if (context.multiplayerSession && context.multiplayerSession->getLobbyState().mode == SandforgeMultiplayerMode::Host)
        {
            _workerPreset = _workerPreset == SandforgeStartWorkerPreset::Standard ? SandforgeStartWorkerPreset::Expanded : SandforgeStartWorkerPreset::Standard;
            context.multiplayerSession->setLobbyMatchSettings(_resourcePreset, _workerPreset);
        }
        return;
    }
    if (_selectedButton == MultiplayerLobbyButton::StartMatch)
    {
        if (context.multiplayerSession)
        {
            context.multiplayerSession->tryStartMatch();
        }
        return;
    }
    if (_selectedButton == MultiplayerLobbyButton::Join)
    {
        context.multiplayerSession->setLocalPlayerName(_localNickname);
        _editingNickname = false;
        _editingAddress = false;
        _editingPort = false;
        if (_joinAddress.empty())
        {
            _joinAddress = "127.0.0.1";
        }
        context.multiplayerSession->join(_joinAddress, _sessionPort);
        return;
    }
    if (_selectedButton == MultiplayerLobbyButton::Ready)
    {
        _editingNickname = false;
        _editingAddress = false;
        _editingPort = false;
        const SandforgeLobbyState& state = context.multiplayerSession->getLobbyState();
        context.multiplayerSession->setLocalReady(!state.localReady);
        return;
    }

    if (context.multiplayerSession)
    {
        context.multiplayerSession->disconnect();
    }
    window.setScene(make_unique<TitleScene>());
}

void MultiplayerLobbyScene::handleNicknameEditing(GameContext& context)
{
    constexpr size_t kMaxNameLength = 16;
    appendPressedLetter(context.input, GLFW_KEY_A, 'A', _localNickname, kMaxNameLength);
    appendPressedLetter(context.input, GLFW_KEY_B, 'B', _localNickname, kMaxNameLength);
    appendPressedLetter(context.input, GLFW_KEY_C, 'C', _localNickname, kMaxNameLength);
    appendPressedLetter(context.input, GLFW_KEY_D, 'D', _localNickname, kMaxNameLength);
    appendPressedLetter(context.input, GLFW_KEY_E, 'E', _localNickname, kMaxNameLength);
    appendPressedLetter(context.input, GLFW_KEY_F, 'F', _localNickname, kMaxNameLength);
    appendPressedLetter(context.input, GLFW_KEY_G, 'G', _localNickname, kMaxNameLength);
    appendPressedLetter(context.input, GLFW_KEY_H, 'H', _localNickname, kMaxNameLength);
    appendPressedLetter(context.input, GLFW_KEY_I, 'I', _localNickname, kMaxNameLength);
    appendPressedLetter(context.input, GLFW_KEY_J, 'J', _localNickname, kMaxNameLength);
    appendPressedLetter(context.input, GLFW_KEY_K, 'K', _localNickname, kMaxNameLength);
    appendPressedLetter(context.input, GLFW_KEY_L, 'L', _localNickname, kMaxNameLength);
    appendPressedLetter(context.input, GLFW_KEY_M, 'M', _localNickname, kMaxNameLength);
    appendPressedLetter(context.input, GLFW_KEY_N, 'N', _localNickname, kMaxNameLength);
    appendPressedLetter(context.input, GLFW_KEY_O, 'O', _localNickname, kMaxNameLength);
    appendPressedLetter(context.input, GLFW_KEY_P, 'P', _localNickname, kMaxNameLength);
    appendPressedLetter(context.input, GLFW_KEY_Q, 'Q', _localNickname, kMaxNameLength);
    appendPressedLetter(context.input, GLFW_KEY_R, 'R', _localNickname, kMaxNameLength);
    appendPressedLetter(context.input, GLFW_KEY_S, 'S', _localNickname, kMaxNameLength);
    appendPressedLetter(context.input, GLFW_KEY_T, 'T', _localNickname, kMaxNameLength);
    appendPressedLetter(context.input, GLFW_KEY_U, 'U', _localNickname, kMaxNameLength);
    appendPressedLetter(context.input, GLFW_KEY_V, 'V', _localNickname, kMaxNameLength);
    appendPressedLetter(context.input, GLFW_KEY_W, 'W', _localNickname, kMaxNameLength);
    appendPressedLetter(context.input, GLFW_KEY_X, 'X', _localNickname, kMaxNameLength);
    appendPressedLetter(context.input, GLFW_KEY_Y, 'Y', _localNickname, kMaxNameLength);
    appendPressedLetter(context.input, GLFW_KEY_Z, 'Z', _localNickname, kMaxNameLength);
    appendPressedDigit(context.input, GLFW_KEY_0, GLFW_KEY_KP_0, '0', _localNickname, kMaxNameLength);
    appendPressedDigit(context.input, GLFW_KEY_1, GLFW_KEY_KP_1, '1', _localNickname, kMaxNameLength);
    appendPressedDigit(context.input, GLFW_KEY_2, GLFW_KEY_KP_2, '2', _localNickname, kMaxNameLength);
    appendPressedDigit(context.input, GLFW_KEY_3, GLFW_KEY_KP_3, '3', _localNickname, kMaxNameLength);
    appendPressedDigit(context.input, GLFW_KEY_4, GLFW_KEY_KP_4, '4', _localNickname, kMaxNameLength);
    appendPressedDigit(context.input, GLFW_KEY_5, GLFW_KEY_KP_5, '5', _localNickname, kMaxNameLength);
    appendPressedDigit(context.input, GLFW_KEY_6, GLFW_KEY_KP_6, '6', _localNickname, kMaxNameLength);
    appendPressedDigit(context.input, GLFW_KEY_7, GLFW_KEY_KP_7, '7', _localNickname, kMaxNameLength);
    appendPressedDigit(context.input, GLFW_KEY_8, GLFW_KEY_KP_8, '8', _localNickname, kMaxNameLength);
    appendPressedDigit(context.input, GLFW_KEY_9, GLFW_KEY_KP_9, '9', _localNickname, kMaxNameLength);
    if (context.input.wasKeyPressed(GLFW_KEY_MINUS) && _localNickname.size() < kMaxNameLength)
    {
        appendNicknameCharacter('-');
    }
    if (context.input.wasKeyPressed(GLFW_KEY_BACKSPACE) && !_localNickname.empty())
    {
        _localNickname.pop_back();
    }
    if (_localNickname.empty())
    {
        _localNickname = "Player";
    }
    if (context.multiplayerSession)
    {
        context.multiplayerSession->setLocalPlayerName(_localNickname);
    }
    if (context.input.wasKeyPressed(GLFW_KEY_ESCAPE))
    {
        _editingNickname = false;
        return;
    }
    if (context.input.wasKeyPressed(GLFW_KEY_ENTER) || context.input.wasKeyPressed(GLFW_KEY_SPACE))
    {
        _editingNickname = false;
        _selectedButton = MultiplayerLobbyButton::AddressField;
    }
}

void MultiplayerLobbyScene::handleAddressEditing(GameContext& context)
{
    constexpr size_t kMaxAddressLength = 21;
    appendPressedDigit(context.input, GLFW_KEY_0, GLFW_KEY_KP_0, '0', _joinAddress, kMaxAddressLength);
    appendPressedDigit(context.input, GLFW_KEY_1, GLFW_KEY_KP_1, '1', _joinAddress, kMaxAddressLength);
    appendPressedDigit(context.input, GLFW_KEY_2, GLFW_KEY_KP_2, '2', _joinAddress, kMaxAddressLength);
    appendPressedDigit(context.input, GLFW_KEY_3, GLFW_KEY_KP_3, '3', _joinAddress, kMaxAddressLength);
    appendPressedDigit(context.input, GLFW_KEY_4, GLFW_KEY_KP_4, '4', _joinAddress, kMaxAddressLength);
    appendPressedDigit(context.input, GLFW_KEY_5, GLFW_KEY_KP_5, '5', _joinAddress, kMaxAddressLength);
    appendPressedDigit(context.input, GLFW_KEY_6, GLFW_KEY_KP_6, '6', _joinAddress, kMaxAddressLength);
    appendPressedDigit(context.input, GLFW_KEY_7, GLFW_KEY_KP_7, '7', _joinAddress, kMaxAddressLength);
    appendPressedDigit(context.input, GLFW_KEY_8, GLFW_KEY_KP_8, '8', _joinAddress, kMaxAddressLength);
    appendPressedDigit(context.input, GLFW_KEY_9, GLFW_KEY_KP_9, '9', _joinAddress, kMaxAddressLength);

    if ((context.input.wasKeyPressed(GLFW_KEY_PERIOD) || context.input.wasKeyPressed(GLFW_KEY_KP_DECIMAL)) &&
        _joinAddress.size() < kMaxAddressLength)
    {
        appendAddressCharacter('.');
    }

    if (context.input.wasKeyPressed(GLFW_KEY_BACKSPACE) && !_joinAddress.empty())
    {
        _joinAddress.pop_back();
    }

    if (context.input.wasKeyPressed(GLFW_KEY_ESCAPE))
    {
        if (_joinAddress.empty())
        {
            _joinAddress = "127.0.0.1";
        }
        _editingAddress = false;
        return;
    }

    if (context.input.wasKeyPressed(GLFW_KEY_ENTER) || context.input.wasKeyPressed(GLFW_KEY_SPACE))
    {
        _editingAddress = false;
        _selectedButton = MultiplayerLobbyButton::Join;
        if (_joinAddress.empty())
        {
            _joinAddress = "127.0.0.1";
        }
        context.multiplayerSession->join(_joinAddress, _sessionPort);
    }
}

void MultiplayerLobbyScene::handlePortEditing(GameContext& context)
{
    string portText = to_string(_sessionPort);
    constexpr size_t kMaxPortLength = 5;
    appendPressedDigit(context.input, GLFW_KEY_0, GLFW_KEY_KP_0, '0', portText, kMaxPortLength);
    appendPressedDigit(context.input, GLFW_KEY_1, GLFW_KEY_KP_1, '1', portText, kMaxPortLength);
    appendPressedDigit(context.input, GLFW_KEY_2, GLFW_KEY_KP_2, '2', portText, kMaxPortLength);
    appendPressedDigit(context.input, GLFW_KEY_3, GLFW_KEY_KP_3, '3', portText, kMaxPortLength);
    appendPressedDigit(context.input, GLFW_KEY_4, GLFW_KEY_KP_4, '4', portText, kMaxPortLength);
    appendPressedDigit(context.input, GLFW_KEY_5, GLFW_KEY_KP_5, '5', portText, kMaxPortLength);
    appendPressedDigit(context.input, GLFW_KEY_6, GLFW_KEY_KP_6, '6', portText, kMaxPortLength);
    appendPressedDigit(context.input, GLFW_KEY_7, GLFW_KEY_KP_7, '7', portText, kMaxPortLength);
    appendPressedDigit(context.input, GLFW_KEY_8, GLFW_KEY_KP_8, '8', portText, kMaxPortLength);
    appendPressedDigit(context.input, GLFW_KEY_9, GLFW_KEY_KP_9, '9', portText, kMaxPortLength);

    if (context.input.wasKeyPressed(GLFW_KEY_BACKSPACE) && !portText.empty())
    {
        portText.pop_back();
    }

    if (context.input.wasKeyPressed(GLFW_KEY_ESCAPE))
    {
        _editingPort = false;
        return;
    }

    if (!portText.empty())
    {
        const int parsedPort = stoi(portText);
        _sessionPort = static_cast<uint16_t>(glm::clamp(parsedPort, 1, 65535));
    }

    if (context.input.wasKeyPressed(GLFW_KEY_ENTER) || context.input.wasKeyPressed(GLFW_KEY_SPACE))
    {
        _editingPort = false;
        _selectedButton = MultiplayerLobbyButton::Join;
    }
}

void MultiplayerLobbyScene::appendAddressCharacter(char character)
{
    if ((character >= '0' && character <= '9') || character == '.')
    {
        _joinAddress.push_back(character);
    }
}

void MultiplayerLobbyScene::appendNicknameCharacter(char character)
{
    if ((character >= 'A' && character <= 'Z') || (character >= '0' && character <= '9') || character == '-')
    {
        _localNickname.push_back(character);
    }
}

MultiplayerLobbyButton MultiplayerLobbyScene::hitTestButton(const vec2& cursorPosition, bool& hit) const
{
    const array<pair<shared_ptr<Sprite>, MultiplayerLobbyButton>, 10> buttons =
    {{
        { _hostButton, MultiplayerLobbyButton::Host },
        { _nicknameField, MultiplayerLobbyButton::NicknameField },
        { _addressField, MultiplayerLobbyButton::AddressField },
        { _portField, MultiplayerLobbyButton::PortField },
        { _resourceField, MultiplayerLobbyButton::ResourcePresetField },
        { _workerField, MultiplayerLobbyButton::WorkerPresetField },
        { _startButton, MultiplayerLobbyButton::StartMatch },
        { _joinButton, MultiplayerLobbyButton::Join },
        { _readyButton, MultiplayerLobbyButton::Ready },
        { _backButton, MultiplayerLobbyButton::Back }
    }};

    for (const auto& [sprite, buttonId] : buttons)
    {
        if (sprite == nullptr) continue;
        const vec2 pos = sprite->GetPosition();
        const vec2 size = sprite->GetScale();
        if (cursorPosition.x >= pos.x && cursorPosition.x <= pos.x + size.x &&
            cursorPosition.y >= pos.y && cursorPosition.y <= pos.y + size.y)
        {
            hit = true;
            return buttonId;
        }
    }

    hit = false;
    return MultiplayerLobbyButton::Host;
}

wstring MultiplayerLobbyScene::buildStatusText(const GameContext& context) const
{
    if (!context.multiplayerSession)
    {
        return L"Session not initialized.";
    }

    const SandforgeLobbyState& state = context.multiplayerSession->getLobbyState();
    string status = state.statusText;
    status += " | LocalReady: ";
    status += state.localReady ? "Yes" : "No";
    status += " RemoteReady: ";
    status += state.remoteReady ? "Yes" : "No";
    if (state.localPlayerId != 0)
    {
        status += " PlayerId: " + to_string(state.localPlayerId);
    }
    status += " | ";
    status += state.localPlayerName.empty() ? _localNickname : state.localPlayerName;
    status += " vs ";
    status += state.remotePlayerName.empty() ? "Remote" : state.remotePlayerName;
    status += " | Resources: ";
    status += state.resourcePreset == SandforgeStartResourcePreset::Rich ? "Rich" : "Standard";
    status += " Workers: ";
    status += state.workerPreset == SandforgeStartWorkerPreset::Expanded ? "Expanded" : "Standard";
    status += " JoinIP: ";
    status += _joinAddress.empty() ? "127.0.0.1" : _joinAddress;
    status += ":";
    status += to_string(_sessionPort);

    return wstring(status.begin(), status.end());
}

wstring MultiplayerLobbyScene::buildNicknameText() const
{
    string text = "Name: " + (_localNickname.empty() ? string("Player") : _localNickname);
    if (_editingNickname)
    {
        text += "_";
    }
    return wstring(text.begin(), text.end());
}

wstring MultiplayerLobbyScene::buildAddressText() const
{
    string text = "Address: ";
    text += _joinAddress.empty() ? "127.0.0.1" : _joinAddress;
    if (_editingAddress)
    {
        text += "_";
    }
    return wstring(text.begin(), text.end());
}

wstring MultiplayerLobbyScene::buildPortText() const
{
    string text = "Port: " + to_string(_sessionPort);
    if (_editingPort)
    {
        text += "_";
    }
    return wstring(text.begin(), text.end());
}

wstring MultiplayerLobbyScene::buildResourcePresetText() const
{
    string text = "Resources: ";
    text += _resourcePreset == SandforgeStartResourcePreset::Rich ? "Rich" : "Standard";
    return wstring(text.begin(), text.end());
}

wstring MultiplayerLobbyScene::buildWorkerPresetText() const
{
    string text = "Workers: ";
    text += _workerPreset == SandforgeStartWorkerPreset::Expanded ? "Expanded" : "Standard";
    return wstring(text.begin(), text.end());
}
