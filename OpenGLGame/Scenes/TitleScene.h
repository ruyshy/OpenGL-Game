#pragma once

#ifndef TITLESCENE_H_
#define TITLESCENE_H_

#include "GameScene.h"
#include "TextEngine.h"

class Shader;
class Sprite;

enum class TitleMenuButton
{
    SinglePlayer,
    MultiPlayer,
    HowToPlay
};

class TitleScene : public GameScene
{
public:
    void onExit(MainWindow& window, GameContext& context) override;
    void onEnter(MainWindow& window, GameContext& context) override;
    void update(MainWindow& window, GameContext& context, double deltaTime) override;
    void render(MainWindow& window, GameContext& context) override;
    void onResize(MainWindow& window, GameContext& context, int width, int height) override;

private:
    bool initializeTextEngine();
    void initializeMenuArt();
    void updateButtonLayout(MainWindow& window);
    void activateButton(MainWindow& window);
    TitleMenuButton hitTestButton(const vec2& cursorPosition, bool& hit) const;

private:
    TextEngine _textEngine;
    bool _textReady = false;
    shared_ptr<Shader> _menuShader;
    shared_ptr<Sprite> _menuBackdrop;
    shared_ptr<Sprite> _menuTitlePanel;
    shared_ptr<Sprite> _singlePlayerButton;
    shared_ptr<Sprite> _multiPlayerButton;
    shared_ptr<Sprite> _howToPlayButton;
    TitleMenuButton _selectedButton = TitleMenuButton::SinglePlayer;
    wstring _menuStatus = L"\uBA54\uB274\uB97C \uC120\uD0DD\uD558\uC138\uC694.";
};

#endif // !TITLESCENE_H_
