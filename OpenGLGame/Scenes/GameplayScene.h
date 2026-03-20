#pragma once

#ifndef GAMEPLAYSCENE_H_
#define GAMEPLAYSCENE_H_

#include "GameScene.h"
#include "GameplayState.h"
#include "TextEngine.h"

class Shader;
class Sprite;

class GameplayScene : public GameScene
{
public:
    void onEnter(MainWindow& window, GameContext& context) override;
    void onExit(MainWindow& window, GameContext& context) override;
    void update(MainWindow& window, GameContext& context, double deltaTime) override;
    void render(MainWindow& window, GameContext& context) override;
    void onResize(MainWindow& window, GameContext& context, int width, int height) override;

private:
    enum class PauseMenuAction
    {
        None,
        Resume,
        ReturnToTitle
    };

    PauseMenuAction hitTestPauseMenu(const vec2& cursorScreenPosition) const;
    void initializePauseArt();

private:
    shared_ptr<Shader> _pauseShader;
    shared_ptr<Sprite> _pauseBackdrop;
    shared_ptr<Sprite> _pauseWindow;
    shared_ptr<Sprite> _pauseHeader;
    shared_ptr<Sprite> _pauseResumeButton;
    shared_ptr<Sprite> _pauseTitleButton;
    GameplayState _state;
    TextEngine _textEngine;
    bool _textReady = false;
    float _cameraX = 0.0f;
    bool _isPaused = false;
    PauseMenuAction _hoveredPauseAction = PauseMenuAction::None;
};

#endif // !GAMEPLAYSCENE_H_
