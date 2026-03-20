#pragma once

#include "GameplayState.h"

class GameplayBuildPreviewController
{
public:
    void beginBuildPreview(GameplayState& state, SandforgeBuildPreviewKind kind, const vec2& cursorWorldPosition) const;
    void cancelBuildPreview(GameplayState& state) const;
    bool tryPlacePreviewAt(GameplayState& state, const vec2& cursorWorldPosition) const;
    bool isBuildPreviewValid(const GameplayState& state, const vec2& cursorWorldPosition) const;
    SandforgeBuildingType getPreviewBuildingType(const GameplayState& state) const;
};
