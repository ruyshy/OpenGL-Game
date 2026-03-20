#include "pch.h"
#include "SandforgeWorld.h"

namespace
{
    constexpr float kNodeCaptureRadius = 40.0f;
    constexpr float kTargetAcquireRadius = 340.0f;
    constexpr float kTowerAcquireRadiusBonus = 120.0f;
    constexpr float kUnitSpawnOffset = 70.0f;
    constexpr float kWorldWidth = 2560.0f;

    SandforgeVec2 moveTowards(const SandforgeVec2& from, const SandforgeVec2& target, float distance)
    {
        const vec2 start = sandforgeToGlm(from);
        const vec2 end = sandforgeToGlm(target);
        const vec2 delta = end - start;
        const float length = glm::length(delta);
        if (length <= 0.001f || length <= distance)
        {
            return target;
        }

        return sandforgeFromGlm(start + ((delta / length) * distance));
    }

    float armorMultiplier(SandforgeDamageType damageType, SandforgeArmorType armorType)
    {
        if (damageType == SandforgeDamageType::Siege && armorType == SandforgeArmorType::Structure) return 1.35f;
        if (damageType == SandforgeDamageType::Piercing && armorType == SandforgeArmorType::Armored) return 0.9f;
        return 1.0f;
    }

    bool isInvulnerableWorker(const SandforgeUnit& unit)
    {
        return unit.alive && unit.unitType == SandforgeUnitType::Worker;
    }

    bool isInvulnerableCombatProductionBuilding(const SandforgeBuilding& building)
    {
        return building.alive &&
            (building.buildingType == SandforgeBuildingType::Barracks ||
             building.buildingType == SandforgeBuildingType::Factory);
    }
}

const vector<SandforgeUnit>& SandforgeWorld::getUnits() const { return _units; }
const vector<SandforgeBuilding>& SandforgeWorld::getBuildings() const { return _buildings; }
const vector<SandforgeResourceNode>& SandforgeWorld::getNodes() const { return _nodes; }
const vector<SandforgeCombatEffect>& SandforgeWorld::getCombatEffects() const { return _combatEffects; }
const SandforgePlayerState& SandforgeWorld::getPlayerState(SandforgePlayerId playerId) const { return _players[playerId == 2 ? 1 : 0]; }
SandforgeMatchResult SandforgeWorld::getMatchResult() const { return _matchResult; }
double SandforgeWorld::getElapsedTime() const { return _elapsedTime; }
string SandforgeWorld::getStatusText() const { return _statusText; }

void SandforgeWorld::reset()
{
    _units.clear();
    _buildings.clear();
    _nodes.clear();
    _combatEffects.clear();
    _matchResult = {};
    _elapsedTime = 0.0;
    _incomeAccumulator = 0.0;
    _enemyAiAccumulator = 0.0;
    _nextEntityId = 1;
    _statusText = "Sandforge match started. Capture nodes and out-produce the enemy.";

    const auto& rules = SandforgeDatabase::getMatchRules();
    _players[0] = { 1, rules.startMetal + 180, rules.startEnergy + 40, 0, false };
    _players[1] = { 2, rules.startMetal - 80, rules.startEnergy - 20, 0, false };

    initializeMap();
    initializePlayers();
    refreshControlledNodeCounts();
}

void SandforgeWorld::update(double deltaTime)
{
    if (_matchResult.gameOver) return;

    _combatEffects.clear();
    _elapsedTime += deltaTime;
    updateAi(deltaTime);
    updateCapture(deltaTime);
    updateProduction(deltaTime);
    updateMovement(deltaTime);
    updateCombat(deltaTime);
    updateIncome(deltaTime);
    cleanupDestroyedEntities();
    refreshControlledNodeCounts();
    evaluateVictory();
}

size_t SandforgeWorld::getUnitCountForPlayer(SandforgePlayerId playerId, SandforgeUnitType unitType) const
{
    size_t count = 0;
    for (const auto& unit : _units)
    {
        if (unit.alive && unit.ownerId == playerId && unit.unitType == unitType) ++count;
    }
    return count;
}

const SandforgeBuilding* SandforgeWorld::findPrimaryBuilding(SandforgePlayerId playerId, SandforgeBuildingType type) const
{
    for (const auto& building : _buildings)
    {
        if (building.alive && building.ownerId == playerId && building.buildingType == type) return &building;
    }
    return nullptr;
}

bool SandforgeWorld::queueProduction(SandforgePlayerId playerId, SandforgeBuildingType buildingType, SandforgeUnitType unitType, bool repeat)
{
    SandforgeBuilding* building = findPrimaryBuildingMutable(playerId, buildingType);
    if (building == nullptr)
    {
        _statusText = "Requested building is not available.";
        return false;
    }

    return queueProduction(playerId, building->id, unitType, repeat);
}

bool SandforgeWorld::queueProduction(SandforgePlayerId playerId, SandforgeEntityId buildingId, SandforgeUnitType unitType, bool repeat)
{
    SandforgeBuilding* building = findBuilding(buildingId);
    if (building == nullptr || !building->alive)
    {
        _statusText = "Requested building is not available.";
        return false;
    }
    if (building->ownerId != playerId)
    {
        _statusText = "Requested building is not available.";
        return false;
    }

    const auto& buildingDef = SandforgeDatabase::getBuilding(building->buildingType);
    if (find(buildingDef.producibleUnits.begin(), buildingDef.producibleUnits.end(), unitType) == buildingDef.producibleUnits.end())
    {
        _statusText = "That building cannot produce the selected unit.";
        return false;
    }

    if (building->productionQueue.size() >= 5)
    {
        _statusText = "Production queue is full.";
        return false;
    }

    const auto& unitDef = SandforgeDatabase::getUnit(unitType);
    if (!spendResources(playerId, unitDef.metalCost, unitDef.energyCost))
    {
        _statusText = "Not enough resources.";
        return false;
    }

    building->productionQueue.push_back({ unitType, unitDef.productionTime, unitDef.productionTime, repeat });
    _statusText = buildingDef.displayName + " queued " + unitDef.displayName + ".";
    return true;
}

bool SandforgeWorld::cancelLastProduction(SandforgePlayerId playerId, SandforgeBuildingType buildingType)
{
    SandforgeBuilding* building = findPrimaryBuildingMutable(playerId, buildingType);
    if (building == nullptr)
    {
        _statusText = "Requested building is not available.";
        return false;
    }

    return cancelLastProduction(playerId, building->id);
}

bool SandforgeWorld::cancelLastProduction(SandforgePlayerId playerId, SandforgeEntityId buildingId)
{
    SandforgeBuilding* building = findBuilding(buildingId);
    if (building == nullptr || !building->alive)
    {
        _statusText = "Requested building is not available.";
        return false;
    }
    if (building->ownerId != playerId)
    {
        _statusText = "Requested building is not available.";
        return false;
    }

    if (building->productionQueue.empty())
    {
        _statusText = "Production queue is empty.";
        return false;
    }

    if (building->productionQueue.size() == 1)
    {
        _statusText = "Active production cannot be canceled.";
        return false;
    }

    const SandforgeProductionItem item = building->productionQueue.back();
    building->productionQueue.pop_back();

    const auto& unitDef = SandforgeDatabase::getUnit(item.unitType);
    SandforgePlayerState& player = _players[playerId == 2 ? 1 : 0];
    player.metal += unitDef.metalCost;
    player.energy += unitDef.energyCost;

    const auto& buildingDef = SandforgeDatabase::getBuilding(building->buildingType);
    _statusText = buildingDef.displayName + " canceled " + unitDef.displayName + ".";
    return true;
}

bool SandforgeWorld::assignWorkerToNode(SandforgePlayerId playerId, size_t nodeIndex)
{
    if (nodeIndex >= _nodes.size()) return false;
    if (_nodes[nodeIndex].ownerId != playerId)
    {
        _statusText = "Workers can only gather from friendly resource objects.";
        return false;
    }

    if (SandforgeUnit* unit = chooseWorkerForNode(playerId, nodeIndex))
    {
        return assignWorkerToNode(playerId, unit->id, nodeIndex);
    }

    _statusText = "No worker is available for that resource object.";
    return false;
}

bool SandforgeWorld::assignWorkerToNode(SandforgePlayerId playerId, SandforgeEntityId workerId, size_t nodeIndex)
{
    if (nodeIndex >= _nodes.size()) return false;
    if (_nodes[nodeIndex].ownerId != playerId)
    {
        _statusText = "Workers can only gather from friendly resource objects.";
        return false;
    }

    SandforgeUnit* unit = findUnit(workerId);
    if (unit == nullptr || !unit->alive || unit->ownerId != playerId || unit->unitType != SandforgeUnitType::Worker)
    {
        _statusText = "Selected worker is not available.";
        return false;
    }

    for (SandforgeResourceNode& node : _nodes)
    {
        if (node.harvestingWorkerId == unit->id)
        {
            node.harvestingWorkerId = 0;
        }
        if (node.capturingWorkerId == unit->id)
        {
            node.capturingWorkerId = 0;
        }
    }

    unit->captureNodeId = _nodes[nodeIndex].id;
    unit->moveTarget = _nodes[nodeIndex].position;
    unit->state = SandforgeUnitState::Capture;
    unit->gatherProgress = 0.0f;
    unit->carriedAmount = 0;
    unit->carriedResourceType = _nodes[nodeIndex].resourceType;
    _statusText = "Selected worker reassigned to a resource object.";
    return true;
}

bool SandforgeWorld::buildNodeHub(SandforgePlayerId playerId, size_t nodeIndex)
{
    if (nodeIndex >= _nodes.size())
    {
        return false;
    }

    const SandforgeResourceNode& node = _nodes[nodeIndex];
    const float offsetY = playerId == 1 ? 96.0f : -96.0f;
    return buildNodeHubAt(playerId, nodeIndex, { node.position.x, node.position.y + offsetY });
}

bool SandforgeWorld::buildNodeHubAt(SandforgePlayerId playerId, size_t nodeIndex, const SandforgeVec2& position)
{
    if (nodeIndex >= _nodes.size())
    {
        return false;
    }

    const SandforgeResourceNode& node = _nodes[nodeIndex];
    if (node.ownerId != playerId)
    {
        _statusText = "Capture the node before building a Node Hub.";
        return false;
    }

    if (findNodeHubNear(node, playerId) != nullptr)
    {
        _statusText = "A Node Hub is already supporting this node.";
        return false;
    }

    if (!canPlaceNodeHub(playerId, nodeIndex, position))
    {
        _statusText = "Invalid Node Hub placement.";
        return false;
    }

    const auto& definition = SandforgeDatabase::getBuilding(SandforgeBuildingType::NodeHub);
    if (!spendResources(playerId, definition.metalCost, definition.energyCost))
    {
        _statusText = "Not enough resources for Node Hub.";
        return false;
    }

    if (spawnBuilding(SandforgeBuildingType::NodeHub, playerId, position) == nullptr)
    {
        _statusText = "Failed to build Node Hub.";
        return false;
    }

    _statusText = definition.displayName + " deployed at " + SandforgeDatabase::getNode(node.resourceType).displayName + ".";
    return true;
}

bool SandforgeWorld::buildFactory(SandforgePlayerId playerId)
{
    const SandforgeBuilding* hq = findPrimaryBuilding(playerId, SandforgeBuildingType::HQ);
    if (hq == nullptr)
    {
        _statusText = "Headquarters is required to build Factory.";
        return false;
    }

    return buildFactoryAt(playerId, { hq->position.x + (playerId == 1 ? 160.0f : -160.0f), hq->position.y + 150.0f });
}

bool SandforgeWorld::buildBarracks(SandforgePlayerId playerId)
{
    const SandforgeBuilding* hq = findPrimaryBuilding(playerId, SandforgeBuildingType::HQ);
    if (hq == nullptr)
    {
        _statusText = "Headquarters is required to build Barracks.";
        return false;
    }

    return buildBarracksAt(playerId, { hq->position.x + (playerId == 1 ? 150.0f : -150.0f), hq->position.y - 150.0f });
}

bool SandforgeWorld::buildBarracksAt(SandforgePlayerId playerId, const SandforgeVec2& buildPosition)
{
    if (!canPlaceBarracks(playerId, buildPosition))
    {
        _statusText = "Invalid Barracks placement.";
        return false;
    }

    const auto& definition = SandforgeDatabase::getBuilding(SandforgeBuildingType::Barracks);
    if (!spendResources(playerId, definition.metalCost, definition.energyCost))
    {
        _statusText = "Not enough resources for Barracks.";
        return false;
    }

    SandforgeBuilding* barracks = spawnBuilding(SandforgeBuildingType::Barracks, playerId, buildPosition);
    if (barracks == nullptr)
    {
        _statusText = "Failed to construct Barracks.";
        return false;
    }

    _statusText = definition.displayName + " online.";
    return true;
}

bool SandforgeWorld::buildFactoryAt(SandforgePlayerId playerId, const SandforgeVec2& buildPosition)
{
    if (findPrimaryBuilding(playerId, SandforgeBuildingType::Factory) != nullptr)
    {
        _statusText = "Factory already constructed.";
        return false;
    }

    const auto& definition = SandforgeDatabase::getBuilding(SandforgeBuildingType::Factory);
    if (!spendResources(playerId, definition.metalCost, definition.energyCost))
    {
        _statusText = "Not enough resources for Factory.";
        return false;
    }

    if (!canPlaceFactory(playerId, buildPosition))
    {
        _statusText = "Invalid Factory placement.";
        return false;
    }

    SandforgeBuilding* factory = spawnBuilding(SandforgeBuildingType::Factory, playerId, buildPosition);
    if (factory == nullptr)
    {
        _statusText = "Failed to construct Factory.";
        return false;
    }

    _statusText = definition.displayName + " online.";
    return true;
}

bool SandforgeWorld::buildDefenseTower(SandforgePlayerId playerId, size_t nodeIndex)
{
    if (nodeIndex >= _nodes.size())
    {
        return false;
    }

    const SandforgeResourceNode& node = _nodes[nodeIndex];
    const float offsetX = playerId == 1 ? -88.0f : 88.0f;
    return buildDefenseTowerAt(playerId, nodeIndex, { node.position.x + offsetX, node.position.y });
}

bool SandforgeWorld::buildDefenseTowerAt(SandforgePlayerId playerId, size_t nodeIndex, const SandforgeVec2& position)
{
    if (nodeIndex >= _nodes.size())
    {
        return false;
    }

    const SandforgeResourceNode& node = _nodes[nodeIndex];
    if (node.ownerId != playerId)
    {
        _statusText = "Capture the node before building a Defense Tower.";
        return false;
    }

    if (findDefenseTowerNear(node, playerId) != nullptr)
    {
        _statusText = "A Defense Tower already guards this node.";
        return false;
    }

    if (!canPlaceDefenseTower(playerId, nodeIndex, position))
    {
        _statusText = "Invalid Defense Tower placement.";
        return false;
    }

    const auto& definition = SandforgeDatabase::getBuilding(SandforgeBuildingType::DefenseTower);
    if (!spendResources(playerId, definition.metalCost, definition.energyCost))
    {
        _statusText = "Not enough resources for Defense Tower.";
        return false;
    }

    if (spawnBuilding(SandforgeBuildingType::DefenseTower, playerId, position) == nullptr)
    {
        _statusText = "Failed to construct Defense Tower.";
        return false;
    }

    _statusText = definition.displayName + " deployed to guard " + SandforgeDatabase::getNode(node.resourceType).displayName + ".";
    return true;
}

bool SandforgeWorld::canPlaceNodeHub(SandforgePlayerId playerId, size_t nodeIndex, const SandforgeVec2& position) const
{
    if (nodeIndex >= _nodes.size())
    {
        return false;
    }

    const SandforgeResourceNode& node = _nodes[nodeIndex];
    if (node.ownerId != playerId)
    {
        return false;
    }

    return sandforgeDistanceSquared(position, node.position) <= (140.0f * 140.0f) &&
        canPlaceBuildingAt(SandforgeBuildingType::NodeHub, playerId, position);
}

bool SandforgeWorld::canPlaceFactory(SandforgePlayerId playerId, const SandforgeVec2& position) const
{
    const SandforgeBuilding* hq = findPrimaryBuilding(playerId, SandforgeBuildingType::HQ);
    if (hq == nullptr)
    {
        return false;
    }

    return sandforgeDistanceSquared(position, hq->position) <= (250.0f * 250.0f) &&
        canPlaceBuildingAt(SandforgeBuildingType::Factory, playerId, position);
}

bool SandforgeWorld::canPlaceBarracks(SandforgePlayerId playerId, const SandforgeVec2& position) const
{
    const SandforgeBuilding* hq = findPrimaryBuilding(playerId, SandforgeBuildingType::HQ);
    if (hq == nullptr)
    {
        return false;
    }

    return sandforgeDistanceSquared(position, hq->position) <= (240.0f * 240.0f) &&
        canPlaceBuildingAt(SandforgeBuildingType::Barracks, playerId, position);
}

bool SandforgeWorld::canPlaceDefenseTower(SandforgePlayerId playerId, size_t nodeIndex, const SandforgeVec2& position) const
{
    if (nodeIndex >= _nodes.size())
    {
        return false;
    }

    const SandforgeResourceNode& node = _nodes[nodeIndex];
    if (node.ownerId != playerId)
    {
        return false;
    }

    return sandforgeDistanceSquared(position, node.position) <= (150.0f * 150.0f) &&
        canPlaceBuildingAt(SandforgeBuildingType::DefenseTower, playerId, position);
}

bool SandforgeWorld::moveUnitTo(SandforgePlayerId playerId, SandforgeEntityId unitId, const SandforgeVec2& position)
{
    SandforgeUnit* unit = findUnit(unitId);
    if (unit == nullptr || !unit->alive || unit->ownerId != playerId)
    {
        _statusText = "Selected unit is not available.";
        return false;
    }

    if (unit->unitType == SandforgeUnitType::Worker)
    {
        _statusText = "Workers can only capture and gather from nodes.";
        return false;
    }

    unit->moveTarget = position;
    unit->targetId = 0;
    unit->captureNodeId = 0;
    unit->state = SandforgeUnitState::Move;
    _statusText = SandforgeDatabase::getUnit(unit->unitType).displayName + " moving.";
    return true;
}

void SandforgeWorld::initializeMap()
{
    const array<tuple<SandforgeResourceType, SandforgeVec2, SandforgePlayerId>, 4> nodes =
    {{
        { SandforgeResourceType::Metal, { 360.0f, 470.0f }, 1 },
        { SandforgeResourceType::Energy, { 460.0f, 250.0f }, 1 },
        { SandforgeResourceType::Metal, { 2200.0f, 470.0f }, 2 },
        { SandforgeResourceType::Energy, { 2100.0f, 250.0f }, 2 }
    }};

    for (const auto& [type, position, ownerId] : nodes)
    {
        const auto& nodeDef = SandforgeDatabase::getNode(type);
        SandforgeResourceNode node;
        node.id = _nextEntityId++;
        node.entityType = SandforgeEntityType::ResourceNode;
        node.position = position;
        node.resourceType = type;
        node.ownerId = ownerId;
        node.captureRequiredTime = nodeDef.captureTime;
        node.captureProgress = nodeDef.captureTime;
        _nodes.push_back(node);
    }
}

void SandforgeWorld::initializePlayers()
{
    const array<SandforgeVec2, 2> hqPositions = { SandforgeVec2{ 180.0f, 360.0f }, SandforgeVec2{ 2380.0f, 360.0f } };
    for (size_t index = 0; index < _players.size(); ++index)
    {
        const SandforgePlayerId playerId = _players[index].playerId;
        spawnBuilding(SandforgeBuildingType::HQ, playerId, hqPositions[index]);

        const auto& rules = SandforgeDatabase::getMatchRules();
        const int startingWorkerCount = playerId == 1 ? rules.startingWorkerCount + 1 : rules.startingWorkerCount;
        for (int workerIndex = 0; workerIndex < startingWorkerCount; ++workerIndex)
        {
            const float offsetX = playerId == 1 ? 40.0f * static_cast<float>(workerIndex) : -40.0f * static_cast<float>(workerIndex);
            spawnUnit(SandforgeUnitType::Worker, playerId, { hqPositions[index].x + offsetX, hqPositions[index].y - 100.0f });
        }
    }
}

void SandforgeWorld::updateAi(double deltaTime)
{
    _enemyAiAccumulator += deltaTime;
    if (_enemyAiAccumulator < 3.5) return;
    _enemyAiAccumulator = 0.0;

    const SandforgePlayerState& aiPlayer = _players[1];
    const size_t workerCount = getUnitCountForPlayer(2, SandforgeUnitType::Worker);
    const size_t soldierCount = getUnitCountForPlayer(2, SandforgeUnitType::Soldier);
    const size_t defenderCount = getUnitCountForPlayer(2, SandforgeUnitType::Defender);
    const size_t raiderCount = getUnitCountForPlayer(2, SandforgeUnitType::Raider);
    const size_t mechCount = getUnitCountForPlayer(2, SandforgeUnitType::RangerMech);
    const size_t siegeCount = getUnitCountForPlayer(2, SandforgeUnitType::SiegeUnit);
    const bool hasBarracks = findPrimaryBuilding(2, SandforgeBuildingType::Barracks) != nullptr;
    const bool hasFactory = findPrimaryBuilding(2, SandforgeBuildingType::Factory) != nullptr;

    size_t bestNodeIndex = _nodes.size();
    int fewestAssignedWorkers = numeric_limits<int>::max();
    for (size_t nodeIndex = 0; nodeIndex < _nodes.size(); ++nodeIndex)
    {
        if (_nodes[nodeIndex].ownerId != 2)
        {
            continue;
        }

        const int assignedWorkers = countAssignedWorkersToNode(2, _nodes[nodeIndex].id);
        if (assignedWorkers < fewestAssignedWorkers)
        {
            fewestAssignedWorkers = assignedWorkers;
            bestNodeIndex = nodeIndex;
        }
    }

    if (bestNodeIndex < _nodes.size())
    {
        assignWorkerToNode(2, bestNodeIndex);
    }

    if (workerCount < 3 && aiPlayer.metal >= 50)
    {
        queueProduction(2, SandforgeBuildingType::HQ, SandforgeUnitType::Worker);
    }

    if (!hasBarracks &&
        _elapsedTime > 18.0 &&
        aiPlayer.metal >= 180)
    {
        buildBarracks(2);
    }

    if (hasBarracks &&
        !hasFactory &&
        _elapsedTime > 150.0 &&
        aiPlayer.metal >= 220 &&
        aiPlayer.energy >= 80)
    {
        buildFactory(2);
    }

    for (size_t nodeIndex = 0; nodeIndex < _nodes.size(); ++nodeIndex)
    {
        if (_nodes[nodeIndex].ownerId != 2)
        {
            continue;
        }

        if (_elapsedTime > 80.0 &&
            aiPlayer.metal >= 180 &&
            aiPlayer.energy >= 60 &&
            findNodeHubNear(_nodes[nodeIndex], 2) == nullptr)
        {
            buildNodeHub(2, nodeIndex);
            break;
        }

        if (_elapsedTime > 170.0 &&
            aiPlayer.metal >= 180 &&
            aiPlayer.energy >= 40 &&
            (soldierCount + defenderCount + mechCount) >= 3 &&
            findDefenseTowerNear(_nodes[nodeIndex], 2) == nullptr)
        {
            buildDefenseTower(2, nodeIndex);
            break;
        }
    }

    if (hasBarracks && soldierCount < 3)
    {
        queueProduction(2, SandforgeBuildingType::Barracks, SandforgeUnitType::Soldier);
    }
    else if (hasBarracks && defenderCount < 2)
    {
        queueProduction(2, SandforgeBuildingType::Barracks, SandforgeUnitType::Defender);
    }
    else if (hasBarracks && _elapsedTime > 110.0 && raiderCount < 1)
    {
        queueProduction(2, SandforgeBuildingType::Barracks, SandforgeUnitType::Raider);
    }

    if (hasFactory && _elapsedTime > 210.0)
    {
        if (mechCount < 2) queueProduction(2, SandforgeBuildingType::Factory, SandforgeUnitType::RangerMech);
        else if (siegeCount < 1) queueProduction(2, SandforgeBuildingType::Factory, SandforgeUnitType::SiegeUnit);
    }
}

void SandforgeWorld::updateCapture(double deltaTime)
{
    for (SandforgeUnit& unit : _units)
    {
        if (!unit.alive || unit.captureNodeId == 0) continue;
        SandforgeResourceNode* node = findNode(unit.captureNodeId);
        if (node == nullptr || !node->alive)
        {
            unit.captureNodeId = 0;
            unit.state = SandforgeUnitState::Idle;
            continue;
        }

        if (node->ownerId != unit.ownerId)
        {
            unit.captureNodeId = 0;
            unit.state = SandforgeUnitState::Idle;
            continue;
        }

        if (unit.state == SandforgeUnitState::Capture)
        {
            node->capturingWorkerId = unit.id;
            unit.moveTarget = node->position;
            if (sandforgeDistanceSquared(unit.position, node->position) <= (kNodeCaptureRadius * kNodeCaptureRadius))
            {
                if (node->capturingWorkerId == unit.id)
                {
                    node->capturingWorkerId = 0;
                }
                node->harvestingWorkerId = unit.id;
                unit.position = node->position;
                unit.state = SandforgeUnitState::Gather;
                unit.gatherProgress = 0.0f;
                unit.carriedAmount = 0;
                unit.carriedResourceType = node->resourceType;
                _statusText = "Worker started gathering.";
            }
            continue;
        }

        if (unit.state == SandforgeUnitState::Gather)
        {
            node->harvestingWorkerId = unit.id;
            unit.position = node->position;
            unit.gatherProgress += static_cast<float>(deltaTime);
            if (unit.gatherProgress >= SandforgeDatabase::getNode(node->resourceType).incomeInterval)
            {
                unit.gatherProgress = 0.0f;
                unit.carriedAmount = calculateNodeIncome(*node);
                unit.carriedResourceType = node->resourceType;
                if (node->harvestingWorkerId == unit.id)
                {
                    node->harvestingWorkerId = 0;
                }
                if (const SandforgeBuilding* hq = findPrimaryBuilding(unit.ownerId, SandforgeBuildingType::HQ))
                {
                    unit.moveTarget = hq->position;
                    unit.state = SandforgeUnitState::ReturnResource;
                    _statusText = "Worker returning resources to HQ.";
                }
            }
            continue;
        }

        if (unit.state == SandforgeUnitState::ReturnResource)
        {
            const SandforgeBuilding* hq = findPrimaryBuilding(unit.ownerId, SandforgeBuildingType::HQ);
            if (hq == nullptr)
            {
                unit.captureNodeId = 0;
                unit.state = SandforgeUnitState::Idle;
                unit.carriedAmount = 0;
                continue;
            }

            unit.moveTarget = hq->position;
            if (sandforgeDistanceSquared(unit.position, hq->position) <= (kNodeCaptureRadius * kNodeCaptureRadius))
            {
                SandforgePlayerState& player = _players[unit.ownerId == 2 ? 1 : 0];
                if (unit.carriedResourceType == SandforgeResourceType::Metal) player.metal += unit.carriedAmount;
                else player.energy += unit.carriedAmount;

                unit.carriedAmount = 0;
                unit.moveTarget = node->position;
                unit.state = SandforgeUnitState::Capture;
                _statusText = "Worker delivered resources to HQ.";
            }
        }
    }
}

void SandforgeWorld::updateProduction(double deltaTime)
{
    for (SandforgeBuilding& building : _buildings)
    {
        if (!building.alive || building.productionQueue.empty()) continue;

        SandforgeProductionItem& item = building.productionQueue.front();
        item.remainingTime -= static_cast<float>(deltaTime);
        if (item.remainingTime > 0.0f) continue;

        SandforgeUnit* unit = spawnUnit(item.unitType, building.ownerId, { building.position.x, building.position.y - kUnitSpawnOffset });
        if (unit != nullptr)
        {
            unit->moveTarget = unit->position;
            unit->state = SandforgeDatabase::getUnit(unit->unitType).isCombatUnit ? SandforgeUnitState::Idle : SandforgeUnitState::Idle;
        }

        const bool repeat = item.repeat;
        const SandforgeUnitType unitType = item.unitType;
        building.productionQueue.pop_front();
        if (repeat) queueProduction(building.ownerId, building.buildingType, unitType, true);
    }
}

void SandforgeWorld::updateMovement(double deltaTime)
{
    for (SandforgeUnit& unit : _units)
    {
        if (!unit.alive) continue;
        if (unit.attackCooldownRemaining > 0.0f) unit.attackCooldownRemaining = (std::max)(0.0f, unit.attackCooldownRemaining - static_cast<float>(deltaTime));

        const auto& definition = SandforgeDatabase::getUnit(unit.unitType);
        if (!definition.isCombatUnit && unit.captureNodeId == 0) continue;

        if (definition.isCombatUnit && unit.targetId == 0 && unit.state != SandforgeUnitState::Move)
        {
            unit.moveTarget = getEnemyFrontline(unit.ownerId);
            unit.state = SandforgeUnitState::Advance;
        }

        if (sandforgeDistanceSquared(unit.position, unit.moveTarget) > 4.0f)
        {
            unit.position = moveTowards(unit.position, unit.moveTarget, unit.moveSpeed * static_cast<float>(deltaTime));
        }
        else if (unit.state == SandforgeUnitState::Move)
        {
            unit.state = definition.isCombatUnit ? SandforgeUnitState::Idle : SandforgeUnitState::Idle;
        }
    }
}

void SandforgeWorld::updateCombat(double deltaTime)
{
    for (SandforgeBuilding& building : _buildings)
    {
        if (!building.alive || building.attackCooldownRemaining <= 0.0f)
        {
            continue;
        }

        building.attackCooldownRemaining = (std::max)(0.0f, building.attackCooldownRemaining - static_cast<float>(deltaTime));
    }

    for (SandforgeUnit& unit : _units)
    {
        if (!unit.alive) continue;
        const auto& definition = SandforgeDatabase::getUnit(unit.unitType);
        if (!definition.isCombatUnit) continue;

        SandforgeEntityId bestTargetId = 0;
        float bestDistance = kTargetAcquireRadius * kTargetAcquireRadius;

        for (const SandforgeUnit& other : _units)
        {
            if (!other.alive || other.ownerId == unit.ownerId) continue;
            if (isInvulnerableWorker(other)) continue;
            const float distance = sandforgeDistanceSquared(unit.position, other.position);
            if (distance < bestDistance)
            {
                bestDistance = distance;
                bestTargetId = other.id;
            }
        }

        for (const SandforgeBuilding& building : _buildings)
        {
            if (!building.alive || building.ownerId == unit.ownerId) continue;
            if (isInvulnerableCombatProductionBuilding(building)) continue;
            const float distance = sandforgeDistanceSquared(unit.position, building.position);
            if ((definition.preferStructureTarget && distance <= bestDistance * 1.25f) || (!definition.preferStructureTarget && distance < bestDistance))
            {
                bestDistance = distance;
                bestTargetId = building.id;
            }
        }

        unit.targetId = bestTargetId;
        SandforgeEntity* target = findEntity(bestTargetId);
        if (target == nullptr || !target->alive)
        {
            unit.state = SandforgeUnitState::Advance;
            continue;
        }

        if (sandforgeDistanceSquared(unit.position, target->position) > (unit.attackRange * unit.attackRange))
        {
            unit.moveTarget = target->position;
            unit.state = SandforgeUnitState::Advance;
            continue;
        }

        unit.state = SandforgeUnitState::Attack;
        if (unit.attackCooldownRemaining > 0.0f) continue;

        float damage = target->entityType == SandforgeEntityType::Building ? unit.structureDamage : unit.attackDamage;
        if (SandforgeUnit* targetUnit = findUnit(target->id))
        {
            if (isInvulnerableWorker(*targetUnit))
            {
                unit.targetId = 0;
                unit.state = SandforgeUnitState::Advance;
                continue;
            }

            targetUnit->hp -= damage * armorMultiplier(unit.damageType, targetUnit->armorType);
            if (targetUnit->hp <= 0.0f)
            {
                targetUnit->alive = false;
                targetUnit->state = SandforgeUnitState::Dead;
            }
        }
        else if (SandforgeBuilding* targetBuilding = findBuilding(target->id))
        {
            if (isInvulnerableCombatProductionBuilding(*targetBuilding))
            {
                unit.targetId = 0;
                unit.state = SandforgeUnitState::Advance;
                continue;
            }

            targetBuilding->hp -= damage * armorMultiplier(unit.damageType, SandforgeArmorType::Structure);
            if (targetBuilding->hp <= 0.0f) targetBuilding->alive = false;
        }

        unit.attackCooldownRemaining = unit.attackCooldown;
    }

    for (SandforgeBuilding& building : _buildings)
    {
        if (!building.alive || building.attackDamage <= 0.0f || building.attackRange <= 0.0f)
        {
            continue;
        }

        SandforgeUnit* bestTarget = nullptr;
        const float acquireRadius = building.attackRange + kTowerAcquireRadiusBonus;
        float bestDistance = acquireRadius * acquireRadius;
        for (SandforgeUnit& other : _units)
        {
            if (!other.alive || other.ownerId == building.ownerId)
            {
                continue;
            }
            if (isInvulnerableWorker(other))
            {
                continue;
            }

            const float distance = sandforgeDistanceSquared(building.position, other.position);
            if (distance <= bestDistance)
            {
                bestDistance = distance;
                bestTarget = &other;
            }
        }

        if (bestTarget == nullptr || building.attackCooldownRemaining > 0.0f)
        {
            continue;
        }

        _combatEffects.push_back({ building.position, bestTarget->position });
        bestTarget->hp -= building.attackDamage;
        if (bestTarget->hp <= 0.0f)
        {
            bestTarget->alive = false;
            bestTarget->state = SandforgeUnitState::Dead;
        }

        building.attackCooldownRemaining = building.attackCooldown;
    }
}

const SandforgeUnit* SandforgeWorld::findUnitById(SandforgeEntityId id) const
{
    for (const auto& unit : _units)
    {
        if (unit.id == id)
        {
            return &unit;
        }
    }

    return nullptr;
}

const SandforgeBuilding* SandforgeWorld::findBuildingById(SandforgeEntityId id) const
{
    for (const auto& building : _buildings)
    {
        if (building.id == id)
        {
            return &building;
        }
    }

    return nullptr;
}

void SandforgeWorld::updateIncome(double deltaTime)
{
    (void)deltaTime;
}

void SandforgeWorld::cleanupDestroyedEntities()
{
    _units.erase(remove_if(_units.begin(), _units.end(), [](const SandforgeUnit& unit) { return !unit.alive; }), _units.end());
    _buildings.erase(remove_if(_buildings.begin(), _buildings.end(), [](const SandforgeBuilding& building) { return !building.alive; }), _buildings.end());
}

void SandforgeWorld::refreshControlledNodeCounts()
{
    _players[0].controlledNodeCount = 0;
    _players[1].controlledNodeCount = 0;
    for (const SandforgeResourceNode& node : _nodes)
    {
        if (node.ownerId == 1) ++_players[0].controlledNodeCount;
        if (node.ownerId == 2) ++_players[1].controlledNodeCount;
    }
}

void SandforgeWorld::evaluateVictory()
{
    bool player1HasHQ = false;
    bool player2HasHQ = false;
    for (const SandforgeBuilding& building : _buildings)
    {
        if (!building.alive || building.buildingType != SandforgeBuildingType::HQ) continue;
        if (building.ownerId == 1) player1HasHQ = true;
        if (building.ownerId == 2) player2HasHQ = true;
    }

    if (!player1HasHQ || !player2HasHQ)
    {
        _matchResult.gameOver = true;
        _matchResult.reason = SandforgeMatchEndReason::HQDestroyed;
        _matchResult.winnerPlayerId = player1HasHQ ? 1 : 2;
        _statusText = _matchResult.winnerPlayerId == 1 ? "Victory! Enemy HQ destroyed." : "Defeat. Your HQ fell.";
        return;
    }

    if (_elapsedTime >= SandforgeDatabase::getMatchRules().timeLimitSeconds)
    {
        _matchResult.gameOver = true;
        _matchResult.reason = SandforgeMatchEndReason::Timeout;
        const int player1Score = _players[0].controlledNodeCount + static_cast<int>(getUnitCountForPlayer(1, SandforgeUnitType::Soldier) + getUnitCountForPlayer(1, SandforgeUnitType::Defender));
        const int player2Score = _players[1].controlledNodeCount + static_cast<int>(getUnitCountForPlayer(2, SandforgeUnitType::Soldier) + getUnitCountForPlayer(2, SandforgeUnitType::Defender));
        _matchResult.winnerPlayerId = player1Score >= player2Score ? 1 : 2;
        _statusText = _matchResult.winnerPlayerId == 1 ? "Victory on timeout." : "Defeat on timeout.";
    }
}

SandforgeUnit* SandforgeWorld::findUnit(SandforgeEntityId id)
{
    for (auto& unit : _units) if (unit.id == id) return &unit;
    return nullptr;
}

SandforgeBuilding* SandforgeWorld::findBuilding(SandforgeEntityId id)
{
    for (auto& building : _buildings) if (building.id == id) return &building;
    return nullptr;
}

SandforgeResourceNode* SandforgeWorld::findNode(SandforgeEntityId id)
{
    for (auto& node : _nodes) if (node.id == id) return &node;
    return nullptr;
}

SandforgeEntity* SandforgeWorld::findEntity(SandforgeEntityId id)
{
    if (SandforgeUnit* unit = findUnit(id)) return unit;
    if (SandforgeBuilding* building = findBuilding(id)) return building;
    if (SandforgeResourceNode* node = findNode(id)) return node;
    return nullptr;
}

SandforgeVec2 SandforgeWorld::getEnemyFrontline(SandforgePlayerId playerId) const
{
    const SandforgeBuilding* enemyHQ = findPrimaryBuilding(playerId == 1 ? 2 : 1, SandforgeBuildingType::HQ);
    if (enemyHQ != nullptr) return enemyHQ->position;
    return { playerId == 1 ? kWorldWidth - 120.0f : 120.0f, 360.0f };
}

SandforgeBuilding* SandforgeWorld::findPrimaryBuildingMutable(SandforgePlayerId playerId, SandforgeBuildingType type)
{
    for (auto& building : _buildings)
    {
        if (building.alive && building.ownerId == playerId && building.buildingType == type) return &building;
    }
    return nullptr;
}

const SandforgeBuilding* SandforgeWorld::findNodeHubNear(const SandforgeResourceNode& node, SandforgePlayerId playerId) const
{
    const auto& definition = SandforgeDatabase::getBuilding(SandforgeBuildingType::NodeHub);
    const float maxDistanceSquared = definition.nodeBonusRadius * definition.nodeBonusRadius;

    for (const SandforgeBuilding& building : _buildings)
    {
        if (!building.alive || building.ownerId != playerId || building.buildingType != SandforgeBuildingType::NodeHub)
        {
            continue;
        }

        if (sandforgeDistanceSquared(building.position, node.position) <= maxDistanceSquared)
        {
            return &building;
        }
    }

    return nullptr;
}

const SandforgeBuilding* SandforgeWorld::findDefenseTowerNear(const SandforgeResourceNode& node, SandforgePlayerId playerId) const
{
    const float maxDistanceSquared = 140.0f * 140.0f;

    for (const SandforgeBuilding& building : _buildings)
    {
        if (!building.alive || building.ownerId != playerId || building.buildingType != SandforgeBuildingType::DefenseTower)
        {
            continue;
        }

        if (sandforgeDistanceSquared(building.position, node.position) <= maxDistanceSquared)
        {
            return &building;
        }
    }

    return nullptr;
}

bool SandforgeWorld::canPlaceBuildingAt(SandforgeBuildingType type, SandforgePlayerId playerId, const SandforgeVec2& position) const
{
    (void)playerId;

    if (position.x < 72.0f || position.x > kWorldWidth - 72.0f || position.y < 72.0f || position.y > 648.0f)
    {
        return false;
    }

    const vec2 size = SandforgeDatabase::getBuilding(type).visuals.spriteSize;
    const float radius = (std::max)(size.x, size.y) * 0.55f;

    for (const SandforgeBuilding& building : _buildings)
    {
        if (!building.alive)
        {
            continue;
        }

        const vec2 otherSize = SandforgeDatabase::getBuilding(building.buildingType).visuals.spriteSize;
        const float otherRadius = (std::max)(otherSize.x, otherSize.y) * 0.55f;
        const float minDistance = radius + otherRadius + 22.0f;
        if (sandforgeDistanceSquared(position, building.position) < (minDistance * minDistance))
        {
            return false;
        }
    }

    for (const SandforgeResourceNode& node : _nodes)
    {
        const float minDistance = radius + 34.0f;
        if (sandforgeDistanceSquared(position, node.position) < (minDistance * minDistance))
        {
            return false;
        }
    }

    return true;
}

SandforgeUnit* SandforgeWorld::spawnUnit(SandforgeUnitType type, SandforgePlayerId ownerId, const SandforgeVec2& position)
{
    const auto& definition = SandforgeDatabase::getUnit(type);
    SandforgeUnit unit;
    unit.id = _nextEntityId++;
    unit.entityType = SandforgeEntityType::Unit;
    unit.ownerId = ownerId;
    unit.position = position;
    unit.unitType = type;
    unit.maxHp = definition.maxHp;
    unit.hp = definition.maxHp;
    unit.attackDamage = definition.damage;
    unit.structureDamage = definition.structureDamage;
    unit.attackRange = definition.attackRange;
    unit.attackCooldown = definition.attackCooldown;
    unit.moveSpeed = definition.moveSpeed;
    unit.damageType = definition.damageType;
    unit.armorType = definition.armorType;
    _units.push_back(unit);
    return &_units.back();
}

SandforgeBuilding* SandforgeWorld::spawnBuilding(SandforgeBuildingType type, SandforgePlayerId ownerId, const SandforgeVec2& position)
{
    const auto& definition = SandforgeDatabase::getBuilding(type);
    SandforgeBuilding building;
    building.id = _nextEntityId++;
    building.entityType = SandforgeEntityType::Building;
    building.ownerId = ownerId;
    building.position = position;
    building.buildingType = type;
    building.maxHp = definition.maxHp;
    building.hp = definition.maxHp;
    building.rallyPoint = { position.x + (ownerId == 1 ? 100.0f : -100.0f), position.y };
    building.attackDamage = definition.attackDamage;
    building.attackRange = definition.attackRange;
    building.attackCooldown = definition.attackCooldown;
    _buildings.push_back(building);
    return &_buildings.back();
}

int SandforgeWorld::calculateNodeIncome(const SandforgeResourceNode& node) const
{
    const auto& nodeDefinition = SandforgeDatabase::getNode(node.resourceType);
    float multiplier = 1.0f;
    for (const SandforgeBuilding& building : _buildings)
    {
        if (!building.alive || building.ownerId != node.ownerId || building.buildingType != SandforgeBuildingType::NodeHub) continue;
        const auto& definition = SandforgeDatabase::getBuilding(building.buildingType);
        if (sandforgeDistanceSquared(building.position, node.position) <= (definition.nodeBonusRadius * definition.nodeBonusRadius))
        {
            multiplier = (std::max)(multiplier, definition.nodeIncomeMultiplier);
        }
    }
    return static_cast<int>(round(static_cast<float>(nodeDefinition.incomeAmount) * multiplier));
}

bool SandforgeWorld::spendResources(SandforgePlayerId playerId, int metal, int energy)
{
    SandforgePlayerState& player = _players[playerId == 2 ? 1 : 0];
    if (player.metal < metal || player.energy < energy) return false;
    player.metal -= metal;
    player.energy -= energy;
    return true;
}

int SandforgeWorld::countAssignedWorkersToNode(SandforgePlayerId playerId, SandforgeEntityId nodeId) const
{
    int count = 0;
    for (const SandforgeUnit& unit : _units)
    {
        if (!unit.alive || unit.ownerId != playerId || unit.unitType != SandforgeUnitType::Worker)
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

SandforgeUnit* SandforgeWorld::chooseWorkerForNode(SandforgePlayerId playerId, size_t nodeIndex)
{
    if (nodeIndex >= _nodes.size())
    {
        return nullptr;
    }

    const SandforgeEntityId targetNodeId = _nodes[nodeIndex].id;
    SandforgeUnit* bestWorker = nullptr;
    float bestScore = numeric_limits<float>::max();

    for (SandforgeUnit& unit : _units)
    {
        if (!unit.alive || unit.ownerId != playerId || unit.unitType != SandforgeUnitType::Worker)
        {
            continue;
        }

        float score = 0.0f;
        if (unit.captureNodeId == 0)
        {
            score = 0.0f;
        }
        else if (unit.captureNodeId == targetNodeId)
        {
            score = 1000.0f;
        }
        else if (unit.state == SandforgeUnitState::ReturnResource)
        {
            score = 25.0f;
        }
        else
        {
            score = 50.0f;
        }

        score += sandforgeDistanceSquared(unit.position, _nodes[nodeIndex].position) * 0.0001f;
        if (score < bestScore)
        {
            bestScore = score;
            bestWorker = &unit;
        }
    }

    return bestWorker;
}
