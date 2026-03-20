#include "pch.h"
#include "TitleScene.h"

#include "GameplayScene.h"
#include "MainWindow.h"

void TitleScene::onEnter(MainWindow& window, GameContext& context)
{
    context.currentSceneName = "TitleScene";

    glClearColor(0.08f, 0.10f, 0.18f, 1.0f);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    onResize(window, context, window.getScreenWidth(), window.getScreenHeight());
}

void TitleScene::update(MainWindow& window, GameContext& context, double deltaTime)
{
    (void)deltaTime;

    if (context.input.wasKeyPressed(GLFW_KEY_ENTER) ||
        context.input.wasKeyPressed(GLFW_KEY_SPACE) ||
        context.input.wasMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT))
    {
        window.setScene(make_unique<GameplayScene>());
    }
}

void TitleScene::render(MainWindow& window, GameContext& context)
{
    (void)window;
    (void)context;

    glClear(GL_COLOR_BUFFER_BIT);
}

void TitleScene::onResize(MainWindow& window, GameContext& context, int width, int height)
{
    (void)window;
    (void)context;

    if (width <= 0 || height <= 0)
    {
        return;
    }

    glViewport(0, 0, width, height);
}
