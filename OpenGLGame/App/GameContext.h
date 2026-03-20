#pragma once

#ifndef GAMECONTEXT_H_
#define GAMECONTEXT_H_

#include "InputState.h"

class SandforgeMultiplayerSession;

struct GameContext
{
    string appName = "OpenGL Game";
    string currentSceneName;
    double totalTime = 0.0;
    int frameCount = 0;
    InputState input;
    shared_ptr<SandforgeMultiplayerSession> multiplayerSession;
};

#endif // !GAMECONTEXT_H_
