#include "pch.h"
#include "GameplaySelectionController.h"

#include "GameplayState.h"

namespace
{
    int countAssignedWorkers(const SandforgeWorld& world, SandforgeEntityId nodeId)
    {
        int count = 0;
        for (const SandforgeUnit& unit : world.getUnits())
        {
            if (!unit.alive || unit.ownerId != 1 || unit.unitType != SandforgeUnitType::Worker)
            {
                continue;
            }

            if (unit.captureNodeId == nodeId)
            {
                ++count;
            }
        }

        return count;
    }
}

void GameplaySelectionController::setSelection(GameplayState& state, SandforgeSelectionKind kind, int index) const
{
    state._selectionKind = kind;
    if (kind != SandforgeSelectionKind::Unit)
    {
        state._selectedUnitId = 0;
    }
    if (kind != SandforgeSelectionKind::Barracks && kind != SandforgeSelectionKind::Factory)
    {
        state._selectedBuildingId = 0;
    }
    if (kind == SandforgeSelectionKind::Node)
    {
        state._selectedNodeIndex = (std::max)(0, (std::min)(index, static_cast<int>(state._world.getNodes().size()) - 1));
    }
}

const SandforgeUnit* GameplaySelectionController::getSelectedUnit(const GameplayState& state) const
{
    return state._world.findUnitById(state._selectedUnitId);
}

const SandforgeBuilding* GameplaySelectionController::getSelectedBuilding(const GameplayState& state) const
{
    if (state._selectedBuildingId == 0)
    {
        return nullptr;
    }

    const SandforgeBuilding* building = state._world.findBuildingById(state._selectedBuildingId);
    if (building == nullptr || !building->alive || building->ownerId != 1)
    {
        return nullptr;
    }

    if (state._selectionKind == SandforgeSelectionKind::Barracks && building->buildingType != SandforgeBuildingType::Barracks)
    {
        return nullptr;
    }
    if (state._selectionKind == SandforgeSelectionKind::Factory && building->buildingType != SandforgeBuildingType::Factory)
    {
        return nullptr;
    }

    return building;
}

void GameplaySelectionController::setBuildingSelection(GameplayState& state, SandforgeSelectionKind kind, SandforgeEntityId buildingId) const
{
    state._selectionKind = kind;
    state._selectedUnitId = 0;
    state._selectedBuildingId = buildingId;
}

string GameplaySelectionController::buildSelectionLabel(const GameplayState& state) const
{
    if (state._selectionKind == SandforgeSelectionKind::Unit)
    {
        const SandforgeUnit* unit = getSelectedUnit(state);
        if (unit != nullptr)
        {
            return SandforgeDatabase::getUnit(unit->unitType).displayName;
        }
    }
    if (state._selectionKind == SandforgeSelectionKind::HQ) return "Headquarters";
    if (state._selectionKind == SandforgeSelectionKind::Barracks)
    {
        if (const SandforgeBuilding* building = getSelectedBuilding(state))
        {
            return "Barracks #" + to_string(building->id);
        }
        return "Barracks";
    }
    if (state._selectionKind == SandforgeSelectionKind::Factory)
    {
        if (const SandforgeBuilding* building = getSelectedBuilding(state))
        {
            return "Factory #" + to_string(building->id);
        }
        return "Factory";
    }
    if (state._selectionKind == SandforgeSelectionKind::NodeHub) return "Node Hub";
    if (state._selectionKind == SandforgeSelectionKind::DefenseTower) return "Defense Tower";
    if (state._selectionKind == SandforgeSelectionKind::Node && state._selectedNodeIndex < static_cast<int>(state._world.getNodes().size()))
    {
        const SandforgeResourceNode& node = state._world.getNodes()[state._selectedNodeIndex];
        return SandforgeDatabase::getNode(node.resourceType).displayName;
    }
    return "None";
}

vector<string> GameplaySelectionController::buildSelectionDetails(const GameplayState& state) const
{
    vector<string> details;
    if (state._selectionKind == SandforgeSelectionKind::Unit)
    {
        const SandforgeUnit* unit = getSelectedUnit(state);
        if (unit != nullptr)
        {
            const auto& definition = SandforgeDatabase::getUnit(unit->unitType);
            details.push_back("Selected  " + definition.displayName + " HP " + to_string(static_cast<int>(unit->hp)) + "/" + to_string(static_cast<int>(unit->maxHp)));
            if (unit->unitType == SandforgeUnitType::Worker)
            {
                details.push_back("Worker  Auto gathers after W/E assignment");
                details.push_back("Hotkeys  W = Metal   E = Energy");
                if (unit->captureNodeId != 0)
                {
                    for (const SandforgeResourceNode& node : state._world.getNodes())
                    {
                        if (node.id == unit->captureNodeId)
                        {
                            details.push_back("Job  " + SandforgeDatabase::getNode(node.resourceType).displayName);
                            break;
                        }
                    }
                }
            }
            else
            {
                details.push_back("Command  Right click to move");
            }
        }
    }
    else if (state._selectionKind == SandforgeSelectionKind::HQ)
    {
        const SandforgeBuilding* hq = state._world.findPrimaryBuilding(1, SandforgeBuildingType::HQ);
        if (hq != nullptr)
        {
            details.push_back("Selected  HQ HP " + to_string(static_cast<int>(hq->hp)) + "/" + to_string(static_cast<int>(hq->maxHp)));
            details.push_back("Actions  [1] Worker  [B] Barracks  [F] Factory");
        }
    }
    else if (state._selectionKind == SandforgeSelectionKind::Barracks)
    {
        const SandforgeBuilding* barracks = getSelectedBuilding(state);
        if (barracks != nullptr)
        {
            details.push_back("Selected  Barracks #" + to_string(barracks->id) + " HP " + to_string(static_cast<int>(barracks->hp)) + "/" + to_string(static_cast<int>(barracks->maxHp)));
            details.push_back("Actions  [2] Soldier  [3] Defender");
        }
    }
    else if (state._selectionKind == SandforgeSelectionKind::Factory)
    {
        const SandforgeBuilding* factory = getSelectedBuilding(state);
        if (factory != nullptr)
        {
            details.push_back("Selected  Factory HP " + to_string(static_cast<int>(factory->hp)) + "/" + to_string(static_cast<int>(factory->maxHp)));
            details.push_back("Actions  [6] Mech  [7] Siege");
        }
    }
    else if (state._selectionKind == SandforgeSelectionKind::NodeHub)
    {
        for (const SandforgeBuilding& building : state._world.getBuildings())
        {
            if (building.alive && building.ownerId == 1 && building.buildingType == SandforgeBuildingType::NodeHub)
            {
                details.push_back("Selected  Node Hub HP " + to_string(static_cast<int>(building.hp)) + "/" + to_string(static_cast<int>(building.maxHp)));
                details.push_back("Bonus  Nearby node income boosted");
                break;
            }
        }
    }
    else if (state._selectionKind == SandforgeSelectionKind::Node && state._selectedNodeIndex < static_cast<int>(state._world.getNodes().size()))
    {
        const SandforgeResourceNode& node = state._world.getNodes()[state._selectedNodeIndex];
        details.push_back("Selected  " + SandforgeDatabase::getNode(node.resourceType).displayName + " owner P" + to_string(node.ownerId));
        details.push_back("Workers  " + to_string(countAssignedWorkers(state._world, node.id)) + " assigned");
        if (node.ownerId == 1)
        {
            details.push_back("Build  [4] Node Hub  [8] Tower");
        }
    }
    else if (state._selectionKind == SandforgeSelectionKind::DefenseTower)
    {
        for (const SandforgeBuilding& building : state._world.getBuildings())
        {
            if (building.alive && building.ownerId == 1 && building.buildingType == SandforgeBuildingType::DefenseTower)
            {
                details.push_back("Selected  Defense Tower HP " + to_string(static_cast<int>(building.hp)) + "/" + to_string(static_cast<int>(building.maxHp)));
                details.push_back("Combat  Auto attacks nearby enemy units");
                break;
            }
        }
    }
    return details;
}

bool GameplaySelectionController::selectObjectAt(GameplayState& state, const vec2& cursorWorldPosition) const
{
    for (const SandforgeUnit& unit : state._world.getUnits())
    {
        if (!unit.alive || unit.ownerId != 1)
        {
            continue;
        }

        const vec2 size = SandforgeDatabase::getUnit(unit.unitType).visuals.spriteSize;
        const vec2 topLeft(unit.position.x - (size.x * 0.5f), unit.position.y - (size.y * 0.5f));
        const vec2 bottomRight = topLeft + size;
        if (cursorWorldPosition.x >= topLeft.x && cursorWorldPosition.x <= bottomRight.x &&
            cursorWorldPosition.y >= topLeft.y && cursorWorldPosition.y <= bottomRight.y)
        {
            state._selectedUnitId = unit.id;
            setSelection(state, SandforgeSelectionKind::Unit);
            state._statusText = "Selected " + SandforgeDatabase::getUnit(unit.unitType).displayName + ".";
            return true;
        }
    }

    const SandforgeBuilding* hq = state._world.findPrimaryBuilding(1, SandforgeBuildingType::HQ);
    if (hq != nullptr)
    {
        const vec2 size = SandforgeDatabase::getBuilding(SandforgeBuildingType::HQ).visuals.spriteSize;
        const vec2 topLeft(hq->position.x - (size.x * 0.5f), hq->position.y - (size.y * 0.5f));
        const vec2 bottomRight = topLeft + size;
        if (cursorWorldPosition.x >= topLeft.x && cursorWorldPosition.x <= bottomRight.x &&
            cursorWorldPosition.y >= topLeft.y && cursorWorldPosition.y <= bottomRight.y)
        {
            setSelection(state, SandforgeSelectionKind::HQ);
            state._statusText = "Selected Headquarters.";
            return true;
        }
    }

    for (const SandforgeBuilding& building : state._world.getBuildings())
    {
        if (!building.alive || building.ownerId != 1 || building.buildingType != SandforgeBuildingType::Barracks)
        {
            continue;
        }

        const vec2 size = SandforgeDatabase::getBuilding(SandforgeBuildingType::Barracks).visuals.spriteSize;
        const vec2 topLeft(building.position.x - (size.x * 0.5f), building.position.y - (size.y * 0.5f));
        const vec2 bottomRight = topLeft + size;
        if (cursorWorldPosition.x >= topLeft.x && cursorWorldPosition.x <= bottomRight.x &&
            cursorWorldPosition.y >= topLeft.y && cursorWorldPosition.y <= bottomRight.y)
        {
            setBuildingSelection(state, SandforgeSelectionKind::Barracks, building.id);
            state._statusText = "Selected Barracks.";
            return true;
        }
    }

    for (const SandforgeBuilding& building : state._world.getBuildings())
    {
        if (!building.alive || building.ownerId != 1 || building.buildingType != SandforgeBuildingType::Factory)
        {
            continue;
        }

        const vec2 size = SandforgeDatabase::getBuilding(SandforgeBuildingType::Factory).visuals.spriteSize;
        const vec2 topLeft(building.position.x - (size.x * 0.5f), building.position.y - (size.y * 0.5f));
        const vec2 bottomRight = topLeft + size;
        if (cursorWorldPosition.x >= topLeft.x && cursorWorldPosition.x <= bottomRight.x &&
            cursorWorldPosition.y >= topLeft.y && cursorWorldPosition.y <= bottomRight.y)
        {
            setBuildingSelection(state, SandforgeSelectionKind::Factory, building.id);
            state._statusText = "Selected Factory.";
            return true;
        }
    }

    for (const SandforgeBuilding& building : state._world.getBuildings())
    {
        if (!building.alive || building.ownerId != 1 || building.buildingType != SandforgeBuildingType::NodeHub)
        {
            continue;
        }

        const vec2 size = SandforgeDatabase::getBuilding(SandforgeBuildingType::NodeHub).visuals.spriteSize;
        const vec2 topLeft(building.position.x - (size.x * 0.5f), building.position.y - (size.y * 0.5f));
        const vec2 bottomRight = topLeft + size;
        if (cursorWorldPosition.x >= topLeft.x && cursorWorldPosition.x <= bottomRight.x &&
            cursorWorldPosition.y >= topLeft.y && cursorWorldPosition.y <= bottomRight.y)
        {
            setSelection(state, SandforgeSelectionKind::NodeHub);
            state._statusText = "Selected Node Hub.";
            return true;
        }
    }

    for (const SandforgeBuilding& building : state._world.getBuildings())
    {
        if (!building.alive || building.ownerId != 1 || building.buildingType != SandforgeBuildingType::DefenseTower)
        {
            continue;
        }

        const vec2 size = SandforgeDatabase::getBuilding(SandforgeBuildingType::DefenseTower).visuals.spriteSize;
        const vec2 topLeft(building.position.x - (size.x * 0.5f), building.position.y - (size.y * 0.5f));
        const vec2 bottomRight = topLeft + size;
        if (cursorWorldPosition.x >= topLeft.x && cursorWorldPosition.x <= bottomRight.x &&
            cursorWorldPosition.y >= topLeft.y && cursorWorldPosition.y <= bottomRight.y)
        {
            setSelection(state, SandforgeSelectionKind::DefenseTower);
            state._statusText = "Selected Defense Tower.";
            return true;
        }
    }

    for (size_t index = 0; index < state._world.getNodes().size(); ++index)
    {
        const SandforgeResourceNode& node = state._world.getNodes()[index];
        const vec2 size = SandforgeDatabase::getNode(node.resourceType).visuals.spriteSize;
        const vec2 topLeft(node.position.x - (size.x * 0.5f), node.position.y - (size.y * 0.5f));
        const vec2 bottomRight = topLeft + size;
        if (cursorWorldPosition.x >= topLeft.x && cursorWorldPosition.x <= bottomRight.x &&
            cursorWorldPosition.y >= topLeft.y && cursorWorldPosition.y <= bottomRight.y)
        {
            setSelection(state, SandforgeSelectionKind::Node, static_cast<int>(index));
            state._statusText = "Selected " + SandforgeDatabase::getNode(node.resourceType).displayName + ".";
            return true;
        }
    }

    return false;
}
