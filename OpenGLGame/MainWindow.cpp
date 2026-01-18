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
	glClearColor(0.0f, 0.5f, 1.0f, 1.0f);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_SCISSOR_TEST);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);

	glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
}

void MainWindow::renderScene()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void MainWindow::updateScene()
{

}

void MainWindow::releaseScene()
{

}

void MainWindow::onWindowSizeChanged(int width, int height)
{

}

void MainWindow::onMouseButtonPressed(int button, int action)
{

}
