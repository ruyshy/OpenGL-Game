#pragma once

#include "GameplayState.h"

class GameplayHudRenderer
{
public:
    void renderHudArt(GameplayState& state) const;
    vector<GameplayState::HudCommandButton> buildHudCommandButtons(const GameplayState& state) const;
    void updateHoveredHudCommand(GameplayState& state, const vec2& cursorScreenPosition) const;
    bool handleHudClick(GameplayState& state, const vec2& cursorScreenPosition, const vec2& cursorWorldPosition) const;
    bool activateHudCommand(GameplayState& state, int hotkey, const vec2& cursorWorldPosition) const;
};
