#pragma once

#ifndef MAINWINDOW_H_
#define MAINWINDOW_H_

#include "OpenGLWindow.h"

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

};


#endif // !MAINWINDOW_H_
