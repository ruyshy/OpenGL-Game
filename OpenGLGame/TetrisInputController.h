#pragma once

#ifndef TETRISINPUTCONTROLLER_H_
#define TETRISINPUTCONTROLLER_H_

#include "pch.h"

class OpenGLWindow;
class TetrisGame;

class TetrisInputController
{
public:
    void reset();
    bool process(OpenGLWindow& window, TetrisGame& game);

private:
    double moveAccumulator_ = 0.0;
    double softDropAccumulator_ = 0.0;
};

#endif // !TETRISINPUTCONTROLLER_H_
