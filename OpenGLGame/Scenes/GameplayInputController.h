#pragma once

#include "GameplayState.h"

class GameplayInputController
{
public:
    void handlePlayerInput(GameplayState& state, const InputState& input, const vec2& cursorScreenPosition, const vec2& cursorWorldPosition) const;
};
