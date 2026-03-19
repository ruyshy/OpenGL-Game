#pragma once

#ifndef MAINWINDOW_H_
#define MAINWINDOW_H_

#include "OpenGLWindow.h"
#include "TetrisAudio.h"
#include "TetrisGame.h"
#include "TetrisInputController.h"
#include "TetrisRenderer.h"

class MainWindow : public OpenGLWindow
{
public:
    MainWindow();
    virtual ~MainWindow();
    virtual void initializeScene();
    virtual void renderScene();
    virtual void updateScene();
    virtual void releaseScene();
    virtual void onWindowSizeChanged(int width, int height);
    virtual void onMouseButtonPressed(int button, int action);

private:
    TetrisAudio audio_;
    TetrisGame game_;
    TetrisInputController inputController_;
    TetrisRenderer renderer_;

private:
    void updateWindowTitle() const;
};

#endif // !MAINWINDOW_H_
