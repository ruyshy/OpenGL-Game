#pragma once

#ifndef MAINWINDOW_H_
#define MAINWINDOW_H_

#include <memory>

#include "OpenGLWindow.h"

class Camera2D;

class MainWindow : public OpenGLWindow
{
public:
    MainWindow();
    virtual ~MainWindow();

    virtual void initializeScene() override;
    virtual void renderScene() override;
    virtual void updateScene() override;
    virtual void releaseScene() override;
    virtual void onWindowSizeChanged(int width, int height) override;
    virtual void onMouseButtonPressed(int button, int action) override;

    std::shared_ptr<Camera2D> GetCamera2D() const;

private:
    std::shared_ptr<Camera2D> mpCamera2D = nullptr;
};

#endif // !MAINWINDOW_H_
