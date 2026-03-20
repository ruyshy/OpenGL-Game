#pragma once

#ifndef TITLESCENE_H_
#define TITLESCENE_H_

#include "GameScene.h"

class TitleScene : public GameScene
{
public:
    void onEnter(MainWindow& window, GameContext& context) override;
    void update(MainWindow& window, GameContext& context, double deltaTime) override;
    void render(MainWindow& window, GameContext& context) override;
    void onResize(MainWindow& window, GameContext& context, int width, int height) override;
};

#endif // !TITLESCENE_H_
