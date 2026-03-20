#include "pch.h"
#include "GameplayInputController.h"

#include "GameplayState.h"
#include "InputState.h"
#include "Sprite.h"

namespace
{
    int findFriendlyResourceNodeIndex(const SandforgeWorld& world, SandforgePlayerId playerId, SandforgeResourceType resourceType)
    {
        for (size_t index = 0; index < world.getNodes().size(); ++index)
        {
            const SandforgeResourceNode& node = world.getNodes()[index];
            if (node.ownerId != playerId || node.resourceType != resourceType)
            {
                continue;
            }

            return static_cast<int>(index);
        }

        return -1;
    }
}

void GameplayInputController::handlePlayerInput(GameplayState& state, const InputState& input, const vec2& cursorScreenPosition, const vec2& cursorWorldPosition) const
{
    if (input.wasKeyPressed(GLFW_KEY_R))
    {
        if (state._multiplayerSession == nullptr)
        {
            state.reset();
        }
        else
        {
            state._statusText = "Reset is disabled during multiplayer.";
        }
        return;
    }

    if (state._buildPreviewKind != SandforgeBuildPreviewKind::None && state._buildPreviewSprite != nullptr)
    {
        const auto& definition = SandforgeDatabase::getBuilding(state.getPreviewBuildingType());
        state._buildPreviewSprite->SetPosition(cursorWorldPosition - (definition.visuals.spriteSize * 0.5f));
    }

    if (state._world.getMatchResult().gameOver)
    {
        return;
    }

    if (input.wasMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT))
    {
        if (state.handleHudClick(cursorScreenPosition, cursorWorldPosition))
        {
        }
        else if (state._buildPreviewKind != SandforgeBuildPreviewKind::None)
        {
            state.tryPlacePreviewAt(cursorWorldPosition);
        }
        else
        {
            state.selectObjectAt(cursorWorldPosition);
        }
    }
    if (input.wasMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT))
    {
        if (state._buildPreviewKind != SandforgeBuildPreviewKind::None)
        {
            state.cancelBuildPreview();
        }
        else if (state._selectionKind == SandforgeSelectionKind::Unit)
        {
            state.requestMoveUnit(state._selectedUnitId, sandforgeFromGlm(cursorWorldPosition));
        }
    }

    if (input.wasKeyPressed(GLFW_KEY_1))
    {
        if (state._selectionKind == SandforgeSelectionKind::HQ)
        {
            state.requestQueueProduction(SandforgeBuildingType::HQ, SandforgeUnitType::Worker);
        }
        else
        {
            state._statusText = "Select Headquarters first.";
        }
    }
    if (input.wasKeyPressed(GLFW_KEY_2))
    {
        if (state._selectionKind == SandforgeSelectionKind::Barracks && state._selectedBuildingId != 0)
        {
            state.requestQueueProduction(state._selectedBuildingId, SandforgeBuildingType::Barracks, SandforgeUnitType::Soldier);
        }
        else
        {
            state._statusText = "Select a Barracks first.";
        }
    }
    if (input.wasKeyPressed(GLFW_KEY_3))
    {
        if (state._selectionKind == SandforgeSelectionKind::Barracks && state._selectedBuildingId != 0)
        {
            state.requestQueueProduction(state._selectedBuildingId, SandforgeBuildingType::Barracks, SandforgeUnitType::Defender);
        }
        else
        {
            state._statusText = "Select a Barracks first.";
        }
    }
    if (input.wasKeyPressed(GLFW_KEY_6))
    {
        if (state._selectionKind == SandforgeSelectionKind::Factory && state._selectedBuildingId != 0)
        {
            state.requestQueueProduction(state._selectedBuildingId, SandforgeBuildingType::Factory, SandforgeUnitType::RangerMech);
        }
        else
        {
            state._statusText = "Select a Factory first.";
        }
    }
    if (input.wasKeyPressed(GLFW_KEY_7))
    {
        if (state._selectionKind == SandforgeSelectionKind::Factory && state._selectedBuildingId != 0)
        {
            state.requestQueueProduction(state._selectedBuildingId, SandforgeBuildingType::Factory, SandforgeUnitType::SiegeUnit);
        }
        else
        {
            state._statusText = "Select a Factory first.";
        }
    }
    if (input.wasKeyPressed(GLFW_KEY_F1))
    {
        state._buildPreviewKind = SandforgeBuildPreviewKind::None;
        state.setSelection(SandforgeSelectionKind::HQ);
    }
    if (input.wasKeyPressed(GLFW_KEY_F2))
    {
        state._buildPreviewKind = SandforgeBuildPreviewKind::None;
        for (const SandforgeBuilding& building : state._world.getBuildings())
        {
            if (building.alive && building.ownerId == state.getLocalPlayerId() && building.buildingType == SandforgeBuildingType::Barracks)
            {
                state.setBuildingSelection(SandforgeSelectionKind::Barracks, building.id);
                break;
            }
        }
    }
    if (input.wasKeyPressed(GLFW_KEY_F3))
    {
        state._buildPreviewKind = SandforgeBuildPreviewKind::None;
        for (const SandforgeBuilding& building : state._world.getBuildings())
        {
            if (building.alive && building.ownerId == state.getLocalPlayerId() && building.buildingType == SandforgeBuildingType::Factory)
            {
                state.setBuildingSelection(SandforgeSelectionKind::Factory, building.id);
                break;
            }
        }
    }
    if (input.wasKeyPressed(GLFW_KEY_TAB))
    {
        if (state._selectionKind == SandforgeSelectionKind::HQ)
        {
            for (const SandforgeBuilding& building : state._world.getBuildings())
            {
                if (building.alive && building.ownerId == state.getLocalPlayerId() && building.buildingType == SandforgeBuildingType::Barracks)
                {
                    state.setBuildingSelection(SandforgeSelectionKind::Barracks, building.id);
                    break;
                }
            }
        }
        else if (state._selectionKind == SandforgeSelectionKind::Barracks)
        {
            bool advancedToNextBarracks = false;
            bool foundCurrent = false;
            for (const SandforgeBuilding& building : state._world.getBuildings())
            {
                if (!building.alive || building.ownerId != state.getLocalPlayerId() || building.buildingType != SandforgeBuildingType::Barracks)
                {
                    continue;
                }

                if (foundCurrent)
                {
                    state.setBuildingSelection(SandforgeSelectionKind::Barracks, building.id);
                    advancedToNextBarracks = true;
                    break;
                }

                if (building.id == state._selectedBuildingId)
                {
                    foundCurrent = true;
                }
            }

            if (!advancedToNextBarracks)
            {
                for (const SandforgeBuilding& building : state._world.getBuildings())
                {
                    if (building.alive && building.ownerId == state.getLocalPlayerId() && building.buildingType == SandforgeBuildingType::Factory)
                    {
                        state.setBuildingSelection(SandforgeSelectionKind::Factory, building.id);
                        advancedToNextBarracks = true;
                        break;
                    }
                }
            }

            if (!advancedToNextBarracks)
            {
                const int nodeCount = static_cast<int>(state._world.getNodes().size());
                if (nodeCount > 0) state.setSelection(SandforgeSelectionKind::Node, (state._selectedNodeIndex + 1) % nodeCount);
                else state.setSelection(SandforgeSelectionKind::HQ);
            }
        }
        else if (state._selectionKind == SandforgeSelectionKind::Factory)
        {
            const int nodeCount = static_cast<int>(state._world.getNodes().size());
            if (nodeCount > 0) state.setSelection(SandforgeSelectionKind::Node, (state._selectedNodeIndex + 1) % nodeCount);
        }
        else
        {
            state.setSelection(SandforgeSelectionKind::HQ);
        }
    }
    if (input.wasKeyPressed(GLFW_KEY_W))
    {
        const int nodeIndex = findFriendlyResourceNodeIndex(state._world, state.getLocalPlayerId(), SandforgeResourceType::Metal);
        if (nodeIndex >= 0)
        {
            const SandforgeEntityId selectedWorkerId = state._selectedUnitId;
            const SandforgeUnit* selectedUnit = state._world.findUnitById(selectedWorkerId);
            if (selectedUnit != nullptr && selectedUnit->unitType == SandforgeUnitType::Worker)
            {
                state.requestAssignWorkerToNode(selectedUnit->id, static_cast<size_t>(nodeIndex));
                state.setSelection(SandforgeSelectionKind::Unit);
                state._selectedUnitId = selectedUnit->id;
            }
            else
            {
                state.requestAssignWorkerToNode(static_cast<size_t>(nodeIndex));
                state.setSelection(SandforgeSelectionKind::Node, nodeIndex);
            }
        }
    }
    if (input.wasKeyPressed(GLFW_KEY_E))
    {
        const int nodeIndex = findFriendlyResourceNodeIndex(state._world, state.getLocalPlayerId(), SandforgeResourceType::Energy);
        if (nodeIndex >= 0)
        {
            const SandforgeEntityId selectedWorkerId = state._selectedUnitId;
            const SandforgeUnit* selectedUnit = state._world.findUnitById(selectedWorkerId);
            if (selectedUnit != nullptr && selectedUnit->unitType == SandforgeUnitType::Worker)
            {
                state.requestAssignWorkerToNode(selectedUnit->id, static_cast<size_t>(nodeIndex));
                state.setSelection(SandforgeSelectionKind::Unit);
                state._selectedUnitId = selectedUnit->id;
            }
            else
            {
                state.requestAssignWorkerToNode(static_cast<size_t>(nodeIndex));
                state.setSelection(SandforgeSelectionKind::Node, nodeIndex);
            }
        }
    }
    if (input.wasKeyPressed(GLFW_KEY_4) &&
        state._selectionKind == SandforgeSelectionKind::Node &&
        state._selectedNodeIndex >= 0 &&
        state._selectedNodeIndex < static_cast<int>(state._world.getNodes().size()))
    {
        state.beginBuildPreview(SandforgeBuildPreviewKind::NodeHub, cursorWorldPosition);
    }
    if (input.wasKeyPressed(GLFW_KEY_B) && state._selectionKind == SandforgeSelectionKind::HQ)
    {
        state.beginBuildPreview(SandforgeBuildPreviewKind::Barracks, cursorWorldPosition);
    }
    if (input.wasKeyPressed(GLFW_KEY_F) && state._selectionKind == SandforgeSelectionKind::HQ)
    {
        state.beginBuildPreview(SandforgeBuildPreviewKind::Factory, cursorWorldPosition);
    }
    if (input.wasKeyPressed(GLFW_KEY_8) &&
        state._selectionKind == SandforgeSelectionKind::Node &&
        state._selectedNodeIndex >= 0 &&
        state._selectedNodeIndex < static_cast<int>(state._world.getNodes().size()))
    {
        state.beginBuildPreview(SandforgeBuildPreviewKind::DefenseTower, cursorWorldPosition);
    }
}
