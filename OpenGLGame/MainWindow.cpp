#include "pch.h"
#include "MainWindow.h"

#include "Camera2D.h"

MainWindow::MainWindow()
{
    mpCamera2D = std::make_shared<Camera2D>();
}

MainWindow::~MainWindow()
{
}

void MainWindow::initializeScene()
{
    glClearColor(0.08f, 0.10f, 0.14f, 1.0f);

    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);

    if (mpCamera2D != nullptr)
    {
        mpCamera2D->SetViewportSize(static_cast<float>(std::max(getScreenWidth(), 1)), static_cast<float>(std::max(getScreenHeight(), 1)));
    }
}

void MainWindow::renderScene()
{
    glClear(GL_COLOR_BUFFER_BIT);
}

void MainWindow::updateScene()
{
}

void MainWindow::releaseScene()
{
    mpCamera2D.reset();
}

void MainWindow::onWindowSizeChanged(int width, int height)
{
    if (mpCamera2D != nullptr)
    {
        mpCamera2D->SetViewportSize(static_cast<float>(width), static_cast<float>(height));
    }
}

void MainWindow::onMouseButtonPressed(int button, int action)
{
    (void)button;
    (void)action;
}

std::shared_ptr<Camera2D> MainWindow::GetCamera2D() const
{
    return mpCamera2D;
}
