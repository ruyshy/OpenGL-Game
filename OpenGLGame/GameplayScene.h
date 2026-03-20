#pragma once

#ifndef GAMEPLAYSCENE_H_
#define GAMEPLAYSCENE_H_

#include "GameScene.h"
#include "GameplayState.h"

class GameplayScene : public GameScene
{
public:
    void onEnter(MainWindow& window, GameContext& context) override;
    void update(MainWindow& window, GameContext& context, double deltaTime) override;
    void render(MainWindow& window, GameContext& context) override;
    void onResize(MainWindow& window, GameContext& context, int width, int height) override;

private:
    GameplayState _state;
};

#endif // !GAMEPLAYSCENE_H_
