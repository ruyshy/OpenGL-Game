#pragma once

#ifndef GAMECONTEXT_H_
#define GAMECONTEXT_H_

#include "InputState.h"

struct GameContext
{
    string appName = "OpenGL Game";
    string currentSceneName;
    double totalTime = 0.0;
    int frameCount = 0;
    InputState input;
};

#endif // !GAMECONTEXT_H_
