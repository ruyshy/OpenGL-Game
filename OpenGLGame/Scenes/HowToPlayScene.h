#pragma once

#ifndef HOWTOPLAYSCENE_H_
#define HOWTOPLAYSCENE_H_

#include "GameScene.h"
#include "TextEngine.h"

class Shader;
class Sprite;

class HowToPlayScene : public GameScene
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

private:
    TextEngine _textEngine;
    bool _textReady = false;
    shared_ptr<Shader> _menuShader;
    shared_ptr<Sprite> _backdrop;
    shared_ptr<Sprite> _contentPanel;
    shared_ptr<Sprite> _headerPanel;
};

#endif // !HOWTOPLAYSCENE_H_
