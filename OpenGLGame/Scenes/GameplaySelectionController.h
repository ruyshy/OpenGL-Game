#pragma once

#include "GameplayState.h"

class GameplaySelectionController
{
public:
    void setSelection(GameplayState& state, SandforgeSelectionKind kind, int index = 0) const;
    const SandforgeUnit* getSelectedUnit(const GameplayState& state) const;
    const SandforgeBuilding* getSelectedBuilding(const GameplayState& state) const;
    void setBuildingSelection(GameplayState& state, SandforgeSelectionKind kind, SandforgeEntityId buildingId) const;
    string buildSelectionLabel(const GameplayState& state) const;
    vector<string> buildSelectionDetails(const GameplayState& state) const;
    bool selectObjectAt(GameplayState& state, const vec2& cursorWorldPosition) const;
};
