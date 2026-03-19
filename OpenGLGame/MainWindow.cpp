#include "pch.h"
#include "MainWindow.h"

MainWindow::MainWindow()
{

}

MainWindow::~MainWindow()
{

}

void MainWindow::initializeScene()
{
	glClearColor(0.04f, 0.06f, 0.10f, 1.0f);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_SCISSOR_TEST);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	_puzzleBoard.initialize();
	_puzzleBoard.resize(getScreenWidth(), getScreenHeight());
}

void MainWindow::renderScene()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	_puzzleBoard.render(getOrthoProjectionMatrix());
}

void MainWindow::updateScene()
{
	_puzzleBoard.update(getTimeDelta());

	if (keyPressedOnce(GLFW_KEY_R))
	{
		_puzzleBoard.reset();
	}

	_titleAccumulator += getTimeDelta();
	if (_titleAccumulator >= 0.1)
	{
		_titleAccumulator = 0.0;
		string windowTitle = "Match Puzzle Prototype  |  Score: " + to_string(_puzzleBoard.getScore())
			+ "  |  Combo: x" + to_string(std::max(1, _puzzleBoard.getCombo()))
			+ "  |  Time: " + to_string(_puzzleBoard.getTimeRemainingSeconds())
			+ "s  |  Click: select/swap  |  R: reset";
		if (_puzzleBoard.isGameOver())
		{
			windowTitle = "Time Up!  |  Final Score: " + to_string(_puzzleBoard.getScore()) + "  |  Press R to restart";
		}
		glfwSetWindowTitle(getWindow(), windowTitle.c_str());
	}
}

void MainWindow::releaseScene()
{

}

void MainWindow::onWindowSizeChanged(int width, int height)
{
	_puzzleBoard.resize(width, height);
}

void MainWindow::onMouseButtonPressed(int button, int action)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		_puzzleBoard.handleClick(getOpenGLCursorPosition());
	}
}
