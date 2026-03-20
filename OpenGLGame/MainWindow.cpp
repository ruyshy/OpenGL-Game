#include "pch.h"
#include "MainWindow.h"

#include "TitleScene.h"

MainWindow::MainWindow()
{
	_context.appName = "OpenGL Game";
	setScene(make_unique<TitleScene>());
}

MainWindow::~MainWindow()
{

}

GameContext& MainWindow::getContext()
{
	return _context;
}

const GameContext& MainWindow::getContext() const
{
	return _context;
}

void MainWindow::setScene(unique_ptr<GameScene> nextScene)
{
	_pendingScene = std::move(nextScene);
}

void MainWindow::initializeScene()
{
	_context.totalTime = 0.0;
	_context.frameCount = 0;
	_context.input.beginFrame();
	applyPendingSceneChange();
}

void MainWindow::renderScene()
{
	if (_activeScene != nullptr)
	{
		_activeScene->render(*this, _context);
	}
}

void MainWindow::updateScene()
{
	_context.totalTime += getTimeDelta();
	_context.frameCount++;
	applyPendingSceneChange();
	if (_activeScene != nullptr)
	{
		_activeScene->update(*this, _context, getTimeDelta());
	}
	_context.input.beginFrame();
}

void MainWindow::releaseScene()
{
	if (_activeScene != nullptr)
	{
		_activeScene->onExit(*this, _context);
		_activeScene.reset();
	}

	_pendingScene.reset();
}

void MainWindow::onWindowSizeChanged(int width, int height)
{
	if (_activeScene != nullptr)
	{
		_activeScene->onResize(*this, _context, width, height);
	}
}

void MainWindow::onMouseButtonPressed(int button, int action)
{
	_context.input.setMouseButtonState(button, action != GLFW_RELEASE);
	if (_activeScene != nullptr)
	{
		_activeScene->onMouseButton(*this, _context, button, action);
	}
}

void MainWindow::onMouseWheelScroll(double scrollOffsetX, double scrollOffsetY)
{
	_context.input.addScrollDelta(scrollOffsetX, scrollOffsetY);
}

void MainWindow::onKeyChanged(int key, int action)
{
	_context.input.setKeyState(key, action != GLFW_RELEASE);
	if (_activeScene != nullptr)
	{
		_activeScene->onKey(*this, _context, key, action);
	}
}

void MainWindow::applyPendingSceneChange()
{
	if (_pendingScene == nullptr)
	{
		return;
	}

	if (_activeScene != nullptr)
	{
		_activeScene->onExit(*this, _context);
	}

	_activeScene = std::move(_pendingScene);
	_activeScene->onEnter(*this, _context);
	_activeScene->onResize(*this, _context, getScreenWidth(), getScreenHeight());
}
