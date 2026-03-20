#pragma once

#ifndef MULTIPLAYERLOBBYSCENE_H_
#define MULTIPLAYERLOBBYSCENE_H_

#include "GameScene.h"
#include "TextEngine.h"
#include "World/SandforgeWorld.h"

class Shader;
class Sprite;

enum class MultiplayerLobbyButton
{
    Host,
    NicknameField,
    AddressField,
    PortField,
    ResourcePresetField,
    WorkerPresetField,
    StartMatch,
    Join,
    Ready,
    Back
};

class MultiplayerLobbyScene : public GameScene
{
public:
    void onEnter(MainWindow& window, GameContext& context) override;
    void onExit(MainWindow& window, GameContext& context) override;
    void update(MainWindow& window, GameContext& context, double deltaTime) override;
    void render(MainWindow& window, GameContext& context) override;
    void onResize(MainWindow& window, GameContext& context, int width, int height) override;

private:
    bool initializeTextEngine();
    void initializeArt();
    void updateLayout(MainWindow& window);
    void activateButton(MainWindow& window, GameContext& context);
    void handleNicknameEditing(GameContext& context);
    void handleAddressEditing(GameContext& context);
    void handlePortEditing(GameContext& context);
    void appendAddressCharacter(char character);
    void appendNicknameCharacter(char character);
    MultiplayerLobbyButton hitTestButton(const vec2& cursorPosition, bool& hit) const;
    wstring buildStatusText(const GameContext& context) const;
    wstring buildNicknameText() const;
    wstring buildAddressText() const;
    wstring buildPortText() const;
    wstring buildResourcePresetText() const;
    wstring buildWorkerPresetText() const;

private:
    TextEngine _textEngine;
    bool _textReady = false;
    shared_ptr<Shader> _menuShader;
    shared_ptr<Sprite> _backdrop;
    shared_ptr<Sprite> _panel;
    shared_ptr<Sprite> _hostButton;
    shared_ptr<Sprite> _nicknameField;
    shared_ptr<Sprite> _addressField;
    shared_ptr<Sprite> _portField;
    shared_ptr<Sprite> _resourceField;
    shared_ptr<Sprite> _workerField;
    shared_ptr<Sprite> _startButton;
    shared_ptr<Sprite> _joinButton;
    shared_ptr<Sprite> _readyButton;
    shared_ptr<Sprite> _backButton;
    MultiplayerLobbyButton _selectedButton = MultiplayerLobbyButton::Host;
    string _localNickname = "Player";
    string _joinAddress = "127.0.0.1";
    uint16_t _sessionPort = 42042;
    SandforgeStartResourcePreset _resourcePreset = SandforgeStartResourcePreset::Standard;
    SandforgeStartWorkerPreset _workerPreset = SandforgeStartWorkerPreset::Standard;
    bool _editingNickname = false;
    bool _editingAddress = false;
    bool _editingPort = false;
};

#endif // !MULTIPLAYERLOBBYSCENE_H_
