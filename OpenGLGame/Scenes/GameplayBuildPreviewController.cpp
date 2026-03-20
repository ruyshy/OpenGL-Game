#include "pch.h"
#include "GameplayBuildPreviewController.h"

#include "GameplayState.h"
#include "Sprite.h"

void GameplayBuildPreviewController::beginBuildPreview(GameplayState& state, SandforgeBuildPreviewKind kind, const vec2& cursorWorldPosition) const
{
    state._buildPreviewKind = kind;
    state._buildPreviewNodeIndex = state._selectionKind == SandforgeSelectionKind::Node ? state._selectedNodeIndex : -1;

    const SandforgeBuildingType buildingType = getPreviewBuildingType(state);
    const auto& definition = SandforgeDatabase::getBuilding(buildingType);
    if (state._buildPreviewSprite != nullptr)
    {
        state._buildPreviewSprite->SetScale(definition.visuals.spriteSize);
        state._buildPreviewSprite = state.createStaticSprite(definition.visuals.imagePath, cursorWorldPosition - (definition.visuals.spriteSize * 0.5f), definition.visuals.spriteSize);
    }

    state._statusText = "Build preview active. Left click to place, right click to cancel.";
}

void GameplayBuildPreviewController::cancelBuildPreview(GameplayState& state) const
{
    state._buildPreviewKind = SandforgeBuildPreviewKind::None;
    state._buildPreviewNodeIndex = -1;
    state._statusText = "Build preview canceled.";
}

bool GameplayBuildPreviewController::tryPlacePreviewAt(GameplayState& state, const vec2& cursorWorldPosition) const
{
    if (state._buildPreviewKind == SandforgeBuildPreviewKind::None)
    {
        return false;
    }

    const SandforgeVec2 position = sandforgeFromGlm(cursorWorldPosition);
    bool placed = false;

    if (state._buildPreviewKind == SandforgeBuildPreviewKind::Barracks)
    {
        placed = state.requestBuildPlacement(state._buildPreviewKind, state._buildPreviewNodeIndex, position);
        if (placed)
        {
            for (auto it = state._world.getBuildings().rbegin(); it != state._world.getBuildings().rend(); ++it)
            {
                if (it->alive && it->ownerId == state.getLocalPlayerId() && it->buildingType == SandforgeBuildingType::Barracks)
                {
                    state.setBuildingSelection(SandforgeSelectionKind::Barracks, it->id);
                    break;
                }
            }
        }
    }
    else if (state._buildPreviewKind == SandforgeBuildPreviewKind::Factory)
    {
        placed = state.requestBuildPlacement(state._buildPreviewKind, state._buildPreviewNodeIndex, position);
        if (placed)
        {
            for (auto it = state._world.getBuildings().rbegin(); it != state._world.getBuildings().rend(); ++it)
            {
                if (it->alive && it->ownerId == state.getLocalPlayerId() && it->buildingType == SandforgeBuildingType::Factory)
                {
                    state.setBuildingSelection(SandforgeSelectionKind::Factory, it->id);
                    break;
                }
            }
        }
    }
    else if (state._buildPreviewKind == SandforgeBuildPreviewKind::NodeHub && state._buildPreviewNodeIndex >= 0)
    {
        placed = state.requestBuildPlacement(state._buildPreviewKind, state._buildPreviewNodeIndex, position);
        if (placed) state.setSelection(SandforgeSelectionKind::NodeHub);
    }
    else if (state._buildPreviewKind == SandforgeBuildPreviewKind::DefenseTower && state._buildPreviewNodeIndex >= 0)
    {
        placed = state.requestBuildPlacement(state._buildPreviewKind, state._buildPreviewNodeIndex, position);
        if (placed) state.setSelection(SandforgeSelectionKind::DefenseTower);
    }

    if (placed)
    {
        state._buildPreviewKind = SandforgeBuildPreviewKind::None;
        state._buildPreviewNodeIndex = -1;
    }

    return placed;
}

bool GameplayBuildPreviewController::isBuildPreviewValid(const GameplayState& state, const vec2& cursorWorldPosition) const
{
    const SandforgeVec2 position = sandforgeFromGlm(cursorWorldPosition);

    if (state._buildPreviewKind == SandforgeBuildPreviewKind::Barracks)
    {
        return state._world.canPlaceBarracks(state.getLocalPlayerId(), position);
    }
    if (state._buildPreviewKind == SandforgeBuildPreviewKind::Factory)
    {
        return state._world.canPlaceFactory(state.getLocalPlayerId(), position);
    }
    if (state._buildPreviewKind == SandforgeBuildPreviewKind::NodeHub && state._buildPreviewNodeIndex >= 0)
    {
        return state._world.canPlaceNodeHub(state.getLocalPlayerId(), static_cast<size_t>(state._buildPreviewNodeIndex), position);
    }
    if (state._buildPreviewKind == SandforgeBuildPreviewKind::DefenseTower && state._buildPreviewNodeIndex >= 0)
    {
        return state._world.canPlaceDefenseTower(state.getLocalPlayerId(), static_cast<size_t>(state._buildPreviewNodeIndex), position);
    }

    return false;
}

SandforgeBuildingType GameplayBuildPreviewController::getPreviewBuildingType(const GameplayState& state) const
{
    if (state._buildPreviewKind == SandforgeBuildPreviewKind::Barracks) return SandforgeBuildingType::Barracks;
    if (state._buildPreviewKind == SandforgeBuildPreviewKind::NodeHub) return SandforgeBuildingType::NodeHub;
    if (state._buildPreviewKind == SandforgeBuildPreviewKind::DefenseTower) return SandforgeBuildingType::DefenseTower;
    return SandforgeBuildingType::Factory;
}
