#pragma once

#ifndef MAINWINDOW_H_
#define MAINWINDOW_H_

#include "OpenGLWindow.h"
#include "PuzzleBoard.h"

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
    PuzzleBoard _puzzleBoard;
    double _titleAccumulator = 0.0;
};


#endif // !MAINWINDOW_H_
