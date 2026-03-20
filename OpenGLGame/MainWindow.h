#pragma once

#ifndef MAINWINDOW_H_
#define MAINWINDOW_H_

#include "GameContext.h"
#include "GameScene.h"
#include "OpenGLWindow.h"

class MainWindow : public OpenGLWindow
{
public:
    MainWindow();
    virtual ~MainWindow();
    GameContext& getContext();
    const GameContext& getContext() const;
    void setScene(unique_ptr<GameScene> nextScene);

    virtual void initializeScene();
    virtual void renderScene();
    virtual void updateScene();
    virtual void releaseScene();
    virtual void onWindowSizeChanged(int width, int height);
    virtual void onMouseButtonPressed(int button, int action);
    virtual void onMouseWheelScroll(double scrollOffsetX, double scrollOffsetY);
    virtual void onKeyChanged(int key, int action);

private:
    void applyPendingSceneChange();

private:
    GameContext _context;
    unique_ptr<GameScene> _activeScene;
    unique_ptr<GameScene> _pendingScene;
};


#endif // !MAINWINDOW_H_
