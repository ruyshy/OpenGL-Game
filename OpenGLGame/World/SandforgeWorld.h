#pragma once

#ifndef SANDFORGE_WORLD_H_
#define SANDFORGE_WORLD_H_

#include "../Core/SandforgeTypes.h"
#include "../Data/SandforgeDatabase.h"

struct SandforgeEntity
{
    SandforgeEntityId id = 0;
    SandforgeEntityType entityType = SandforgeEntityType::None;
    SandforgePlayerId ownerId = 0;
    SandforgeVec2 position{};
    bool alive = true;
};

struct SandforgeProductionItem
{
    SandforgeUnitType unitType = SandforgeUnitType::Worker;
    float totalTime = 0.0f;
    float remainingTime = 0.0f;
    bool repeat = false;
};

struct SandforgeUnit : SandforgeEntity
{
    SandforgeUnitType unitType = SandforgeUnitType::Worker;
    SandforgeUnitState state = SandforgeUnitState::Idle;
    float hp = 0.0f;
    float maxHp = 0.0f;
    float attackDamage = 0.0f;
    float structureDamage = 0.0f;
    float attackRange = 0.0f;
    float attackCooldown = 0.0f;
    float attackCooldownRemaining = 0.0f;
    float moveSpeed = 0.0f;
    SandforgeDamageType damageType = SandforgeDamageType::Normal;
    SandforgeArmorType armorType = SandforgeArmorType::Light;
    SandforgeEntityId targetId = 0;
    SandforgeVec2 moveTarget{};
    SandforgeEntityId captureNodeId = 0;
    float gatherProgress = 0.0f;
    int carriedAmount = 0;
    SandforgeResourceType carriedResourceType = SandforgeResourceType::Metal;
};

struct SandforgeBuilding : SandforgeEntity
{
    SandforgeBuildingType buildingType = SandforgeBuildingType::HQ;
    float hp = 0.0f;
    float maxHp = 0.0f;
    bool underConstruction = false;
    SandforgeVec2 rallyPoint{};
    float attackDamage = 0.0f;
    float attackRange = 0.0f;
    float attackCooldown = 0.0f;
    float attackCooldownRemaining = 0.0f;
    deque<SandforgeProductionItem> productionQueue;
};

struct SandforgeResourceNode : SandforgeEntity
{
    SandforgeResourceType resourceType = SandforgeResourceType::Metal;
    float captureProgress = 0.0f;
    float captureRequiredTime = 5.0f;
    SandforgeEntityId capturingWorkerId = 0;
    SandforgeEntityId harvestingWorkerId = 0;
};

struct SandforgeCombatEffect
{
    SandforgeVec2 from{};
    SandforgeVec2 to{};
};

struct SandforgePlayerState
{
    SandforgePlayerId playerId = 0;
    int metal = 0;
    int energy = 0;
    int controlledNodeCount = 0;
    bool defeated = false;
};

struct SandforgeMatchResult
{
    bool gameOver = false;
    SandforgePlayerId winnerPlayerId = 0;
    SandforgeMatchEndReason reason = SandforgeMatchEndReason::None;
};

enum class SandforgeStartResourcePreset : uint8_t
{
    Standard = 0,
    Rich = 1
};

enum class SandforgeStartWorkerPreset : uint8_t
{
    Standard = 0,
    Expanded = 1
};

struct SandforgeMatchSetup
{
    bool symmetricPlayers = false;
    bool aiEnabled = true;
    SandforgeStartResourcePreset resourcePreset = SandforgeStartResourcePreset::Standard;
    SandforgeStartWorkerPreset workerPreset = SandforgeStartWorkerPreset::Standard;
};

struct SandforgeWorldSnapshot
{
    vector<SandforgeUnit> units;
    vector<SandforgeBuilding> buildings;
    vector<SandforgeResourceNode> nodes;
    array<SandforgePlayerState, 2> players{};
    SandforgeMatchResult matchResult;
    double elapsedTime = 0.0;
    string statusText;
    SandforgeEntityId nextEntityId = 1;
};

class SandforgeWorld
{
public:
    void reset();
    void update(double deltaTime);
    void setMatchSetup(const SandforgeMatchSetup& setup);
    const SandforgeMatchSetup& getMatchSetup() const;
    void setAiEnabled(bool enabled);
    bool isAiEnabled() const;
    SandforgeWorldSnapshot buildSnapshot() const;
    void applySnapshot(const SandforgeWorldSnapshot& snapshot);

    bool queueProduction(SandforgePlayerId playerId, SandforgeBuildingType buildingType, SandforgeUnitType unitType, bool repeat = false);
    bool queueProduction(SandforgePlayerId playerId, SandforgeEntityId buildingId, SandforgeUnitType unitType, bool repeat = false);
    bool cancelLastProduction(SandforgePlayerId playerId, SandforgeBuildingType buildingType);
    bool cancelLastProduction(SandforgePlayerId playerId, SandforgeEntityId buildingId);
    bool assignWorkerToNode(SandforgePlayerId playerId, size_t nodeIndex);
    bool assignWorkerToNode(SandforgePlayerId playerId, SandforgeEntityId workerId, size_t nodeIndex);
    bool buildNodeHub(SandforgePlayerId playerId, size_t nodeIndex);
    bool buildNodeHubAt(SandforgePlayerId playerId, size_t nodeIndex, const SandforgeVec2& position);
    bool buildBarracks(SandforgePlayerId playerId);
    bool buildBarracksAt(SandforgePlayerId playerId, const SandforgeVec2& position);
    bool buildFactory(SandforgePlayerId playerId);
    bool buildFactoryAt(SandforgePlayerId playerId, const SandforgeVec2& position);
    bool buildDefenseTower(SandforgePlayerId playerId, size_t nodeIndex);
    bool buildDefenseTowerAt(SandforgePlayerId playerId, size_t nodeIndex, const SandforgeVec2& position);
    bool canPlaceNodeHub(SandforgePlayerId playerId, size_t nodeIndex, const SandforgeVec2& position) const;
    bool canPlaceBarracks(SandforgePlayerId playerId, const SandforgeVec2& position) const;
    bool canPlaceFactory(SandforgePlayerId playerId, const SandforgeVec2& position) const;
    bool canPlaceDefenseTower(SandforgePlayerId playerId, size_t nodeIndex, const SandforgeVec2& position) const;
    bool moveUnitTo(SandforgePlayerId playerId, SandforgeEntityId unitId, const SandforgeVec2& position);

    const vector<SandforgeUnit>& getUnits() const;
    const vector<SandforgeBuilding>& getBuildings() const;
    const vector<SandforgeResourceNode>& getNodes() const;
    const vector<SandforgeCombatEffect>& getCombatEffects() const;
    const SandforgePlayerState& getPlayerState(SandforgePlayerId playerId) const;
    SandforgeMatchResult getMatchResult() const;
    double getElapsedTime() const;
    string getStatusText() const;

    size_t getUnitCountForPlayer(SandforgePlayerId playerId, SandforgeUnitType unitType) const;
    const SandforgeBuilding* findPrimaryBuilding(SandforgePlayerId playerId, SandforgeBuildingType type) const;
    const SandforgeBuilding* findBuildingById(SandforgeEntityId id) const;
    const SandforgeUnit* findUnitById(SandforgeEntityId id) const;

private:
    void initializeMap();
    void initializePlayers();
    void updateAi(double deltaTime);
    void updateCapture(double deltaTime);
    void updateProduction(double deltaTime);
    void updateMovement(double deltaTime);
    void updateCombat(double deltaTime);
    void updateIncome(double deltaTime);
    void cleanupDestroyedEntities();
    void refreshControlledNodeCounts();
    void evaluateVictory();

    SandforgeUnit* findUnit(SandforgeEntityId id);
    SandforgeBuilding* findBuilding(SandforgeEntityId id);
    SandforgeResourceNode* findNode(SandforgeEntityId id);
    SandforgeEntity* findEntity(SandforgeEntityId id);
    SandforgeVec2 getEnemyFrontline(SandforgePlayerId playerId) const;
    SandforgeBuilding* findPrimaryBuildingMutable(SandforgePlayerId playerId, SandforgeBuildingType type);
    const SandforgeBuilding* findNodeHubNear(const SandforgeResourceNode& node, SandforgePlayerId playerId) const;
    const SandforgeBuilding* findDefenseTowerNear(const SandforgeResourceNode& node, SandforgePlayerId playerId) const;
    bool canPlaceBuildingAt(SandforgeBuildingType type, SandforgePlayerId playerId, const SandforgeVec2& position) const;
    SandforgeUnit* spawnUnit(SandforgeUnitType type, SandforgePlayerId ownerId, const SandforgeVec2& position);
    SandforgeBuilding* spawnBuilding(SandforgeBuildingType type, SandforgePlayerId ownerId, const SandforgeVec2& position);
    int calculateNodeIncome(const SandforgeResourceNode& node) const;
    bool spendResources(SandforgePlayerId playerId, int metal, int energy);
    int countAssignedWorkersToNode(SandforgePlayerId playerId, SandforgeEntityId nodeId) const;
    SandforgeUnit* chooseWorkerForNode(SandforgePlayerId playerId, size_t nodeIndex);

private:
    vector<SandforgeUnit> _units;
    vector<SandforgeBuilding> _buildings;
    vector<SandforgeResourceNode> _nodes;
    vector<SandforgeCombatEffect> _combatEffects;
    array<SandforgePlayerState, 2> _players{};
    SandforgeMatchResult _matchResult;
    double _elapsedTime = 0.0;
    double _incomeAccumulator = 0.0;
    double _enemyAiAccumulator = 0.0;
    SandforgeEntityId _nextEntityId = 1;
    string _statusText = "Sandforge MVP match initialized.";
    bool _aiEnabled = true;
    SandforgeMatchSetup _matchSetup{};
};

#endif // !SANDFORGE_WORLD_H_
