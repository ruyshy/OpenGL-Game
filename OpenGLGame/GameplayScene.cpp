#include "pch.h"
#include "GameplayScene.h"

#include "MainWindow.h"
#include "TitleScene.h"

void GameplayScene::onEnter(MainWindow& window, GameContext& context)
{
    context.currentSceneName = "GameplayScene";
    _state.reset();

    glClearColor(0.03f, 0.20f, 0.12f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);

    onResize(window, context, window.getScreenWidth(), window.getScreenHeight());
}

void GameplayScene::update(MainWindow& window, GameContext& context, double deltaTime)
{
    _state.update(context.input, deltaTime);

    switch (_state.getPhase())
    {
    case GameplayPhase::Ready:
        glClearColor(0.03f, 0.20f, 0.12f, 1.0f);
        break;
    case GameplayPhase::Playing:
        glClearColor(0.03f, 0.20f, 0.12f, 1.0f);
        break;
    case GameplayPhase::Paused:
        glClearColor(0.12f, 0.12f, 0.14f, 1.0f);
        break;
    case GameplayPhase::Completed:
        glClearColor(0.18f, 0.16f, 0.05f, 1.0f);
        break;
    }

    context.currentSceneName =
        "GameplayScene | score=" + std::to_string(_state.getScore()) +
        " moves=" + std::to_string(_state.getMoves()) +
        " cursor=(" + std::to_string(_state.getCursor().x) + "," + std::to_string(_state.getCursor().y) + ")";

    if (context.input.wasKeyPressed(GLFW_KEY_ESCAPE) ||
        context.input.wasMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT))
    {
        window.setScene(make_unique<TitleScene>());
    }
}

void GameplayScene::render(MainWindow& window, GameContext& context)
{
    (void)window;
    (void)context;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GameplayScene::onResize(MainWindow& window, GameContext& context, int width, int height)
{
    (void)window;
    (void)context;

    if (width <= 0 || height <= 0)
    {
        return;
    }

    glViewport(0, 0, width, height);
}
