#pragma once

#ifndef GAMEPLAYSTATE_H_
#define GAMEPLAYSTATE_H_

#include "Animation2D.h"
#include "Data/SandforgeDatabase.h"
#include "World/SandforgeWorld.h"

#include <unordered_map>

class InputState;
class Sprite;
class Shader;
class SandforgeMultiplayerSession;
class GameplayHudRenderer;
class GameplaySelectionController;
class GameplayBuildPreviewController;
class GameplayInputController;
class GameplayWorldRenderer;

enum class SandforgeSelectionKind
{
    Unit,
    HQ,
    Barracks,
    Factory,
    NodeHub,
    DefenseTower,
    Node
};

struct SandforgeVisualActor
{
    shared_ptr<Sprite> sprite;
    unique_ptr<Animation2D> animation;
};

enum class SandforgeBuildPreviewKind
{
    None,
    Barracks,
    Factory,
    NodeHub,
    DefenseTower
};

class GameplayState
{
public:
    struct HudCommandButton
    {
        vec2 position{};
        vec2 size{};
        int hotkey = 0;
        string label;
        bool enabled = false;
    };

    void reset();
    void setViewportSize(int width, int height);
    void update(const InputState& input, double deltaTime, const vec2& cursorScreenPosition, const vec2& cursorWorldPosition);
    void render(const mat4& worldProjection, const mat4& uiProjection, double deltaTime);

    double getElapsedTime() const;
    const string& getStatusText() const;
    vector<string> buildHudLines() const;
    string buildTopBarText() const;
    string buildSelectionTitle() const;
    vector<string> buildSelectionSummary() const;
    vector<string> buildCommandHints() const;
    bool hasCommandTooltip() const;
    string buildCommandTooltipTitle() const;
    vector<string> buildCommandTooltipLines() const;
    vec2 getCommandTooltipPosition() const;
    bool cancelSelectedProduction();
    bool isMatchOver() const;
    void setLocalPlayerId(SandforgePlayerId playerId);
    SandforgePlayerId getLocalPlayerId() const;
    SandforgePlayerId getEnemyPlayerId() const;
    void setMultiplayerSession(const shared_ptr<SandforgeMultiplayerSession>& session);
    bool isRemoteClientControlled() const;
    bool requestQueueProduction(SandforgeBuildingType buildingType, SandforgeUnitType unitType);
    bool requestQueueProduction(SandforgeEntityId buildingId, SandforgeBuildingType buildingType, SandforgeUnitType unitType);
    bool requestAssignWorkerToNode(size_t nodeIndex);
    bool requestAssignWorkerToNode(SandforgeEntityId workerId, size_t nodeIndex);
    bool requestMoveUnit(SandforgeEntityId unitId, const SandforgeVec2& position);
    bool requestBuildPlacement(SandforgeBuildPreviewKind kind, int nodeIndex, const SandforgeVec2& position);
    void setInputEnabled(bool enabled);
    void setSimulationEnabled(bool enabled);
    void configureMatch(const SandforgeMatchSetup& setup);
    SandforgeWorld& getWorld();
    const SandforgeWorld& getWorld() const;
    void applyWorldSnapshot(const SandforgeWorldSnapshot& snapshot);

private:
    friend class GameplayHudRenderer;
    friend class GameplaySelectionController;
    friend class GameplayBuildPreviewController;
    friend class GameplayInputController;
    friend class GameplayWorldRenderer;

    void initializeRenderer();
    void initializeStaticArt();
    void syncVisuals();
    void updateVisualActor(unordered_map<SandforgeEntityId, SandforgeVisualActor>& actors, SandforgeEntityId id, const SandforgeVisualDefinition& visualDefinition, const SandforgeVec2& position, const vec2& size);
    shared_ptr<Sprite> createStaticSprite(const string& imagePath, const vec2& position, const vec2& size);
    void renderStaticArt();
    void renderEffects();
    void renderHudArt();
    void renderFloatingHealthBars();
    vector<HudCommandButton> buildHudCommandButtons() const;
    void updateHoveredHudCommand(const vec2& cursorScreenPosition);
    bool handleHudClick(const vec2& cursorScreenPosition, const vec2& cursorWorldPosition);
    bool activateHudCommand(int hotkey, const vec2& cursorWorldPosition);
    void setSelection(SandforgeSelectionKind kind, int index = 0);
    void beginBuildPreview(SandforgeBuildPreviewKind kind, const vec2& cursorWorldPosition);
    void cancelBuildPreview();
    bool tryPlacePreviewAt(const vec2& cursorWorldPosition);
    bool isBuildPreviewValid(const vec2& cursorWorldPosition) const;
    SandforgeBuildingType getPreviewBuildingType() const;
    const SandforgeUnit* getSelectedUnit() const;
    const SandforgeBuilding* getSelectedBuilding() const;
    void setBuildingSelection(SandforgeSelectionKind kind, SandforgeEntityId buildingId);
    string buildSelectionLabel() const;
    vector<string> buildSelectionDetails() const;
    void handlePlayerInput(const InputState& input, const vec2& cursorScreenPosition, const vec2& cursorWorldPosition);
    bool selectObjectAt(const vec2& cursorWorldPosition);

private:
    SandforgeWorld _world;
    double _elapsedTime = 0.0;
    string _statusText = "Sandforge prototype loaded.";
    shared_ptr<Shader> _spriteShader;
    unordered_map<SandforgeEntityId, SandforgeVisualActor> _unitActors;
    unordered_map<SandforgeEntityId, SandforgeVisualActor> _buildingActors;
    unordered_map<SandforgeEntityId, SandforgeVisualActor> _nodeActors;
    vector<shared_ptr<Sprite>> _terrainTiles;
    shared_ptr<Sprite> _topResourcePanel;
    shared_ptr<Sprite> _infoPanel;
    shared_ptr<Sprite> _rightSelectionPanel;
    shared_ptr<Sprite> _minimapPanel;
    shared_ptr<Sprite> _hudPanel;
    shared_ptr<Sprite> _productionPanel;
    shared_ptr<Sprite> _tooltipPanel;
    shared_ptr<Sprite> _friendlySelectionRing;
    shared_ptr<Sprite> _enemySelectionRing;
    shared_ptr<Sprite> _rallyMarker;
    shared_ptr<Sprite> _rallyGuideDot;
    shared_ptr<Sprite> _queueProgressBar;
    shared_ptr<Sprite> _queueProgressFrame;
    shared_ptr<Sprite> _buildPreviewSprite;
    shared_ptr<Sprite> _attackEffectSprite;
    vector<shared_ptr<Sprite>> _hudIcons;
    vector<shared_ptr<Sprite>> _commandSlotBackgrounds;
    vector<shared_ptr<Sprite>> _nodeStatusFrames;
    vector<shared_ptr<Sprite>> _minimapBlips;
    shared_ptr<Sprite> _healthBarFrame;
    shared_ptr<Sprite> _healthBarFill;
    unordered_map<SandforgeUnitType, shared_ptr<Sprite>> _queuePreviewIcons;
    unordered_map<string, shared_ptr<Sprite>> _uiDecorSprites;
    unordered_map<string, shared_ptr<Sprite>> _portraitSprites;
    shared_ptr<Sprite> _activePortrait;
    SandforgeSelectionKind _selectionKind = SandforgeSelectionKind::Barracks;
    int _selectedNodeIndex = 0;
    SandforgeEntityId _selectedUnitId = 0;
    SandforgeEntityId _selectedBuildingId = 0;
    SandforgeBuildPreviewKind _buildPreviewKind = SandforgeBuildPreviewKind::None;
    int _buildPreviewNodeIndex = -1;
    int _hoveredCommandHotkey = 0;
    vec2 _cursorScreenPosition = vec2(0.0f, 0.0f);
    vec2 _cursorWorldPosition = vec2(0.0f, 0.0f);
    vec2 _uiViewportSize = vec2(1280.0f, 720.0f);
    bool _inputEnabled = true;
    bool _simulationEnabled = true;
    SandforgePlayerId _localPlayerId = 1;
    shared_ptr<SandforgeMultiplayerSession> _multiplayerSession;
};

#endif // !GAMEPLAYSTATE_H_
