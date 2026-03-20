#pragma once

#ifndef GAMESCENE_H_
#define GAMESCENE_H_

class MainWindow;
struct GameContext;

class GameScene
{
public:
    virtual ~GameScene() = default;

    virtual void onEnter(MainWindow& window, GameContext& context) {}
    virtual void onExit(MainWindow& window, GameContext& context) {}
    virtual void update(MainWindow& window, GameContext& context, double deltaTime) {}
    virtual void render(MainWindow& window, GameContext& context) {}
    virtual void onResize(MainWindow& window, GameContext& context, int width, int height) {}
    virtual void onMouseButton(MainWindow& window, GameContext& context, int button, int action) {}
    virtual void onKey(MainWindow& window, GameContext& context, int key, int action) {}
};

#endif // !GAMESCENE_H_
