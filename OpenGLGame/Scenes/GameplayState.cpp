#include "pch.h"
#include "GameplayState.h"

#include "GameplayBuildPreviewController.h"
#include "GameplayHudRenderer.h"
#include "GameplaySelectionController.h"
#include "Animation2D.h"
#include "InputState.h"
#include "Shader.h"
#include "Sprite.h"

namespace
{
    constexpr float kWorldWidth = 2560.0f;
    GameplayHudRenderer gHudRenderer;
    GameplaySelectionController gSelectionController;
    GameplayBuildPreviewController gBuildPreviewController;

    const string kSpriteVertexShader = R"(#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec2 aTexCoord;

out vec4 vertexColor;
out vec2 texCoord;

uniform mat4 projection_matrx;
uniform mat4 model_matrx;
uniform float zDepth;
uniform vec4 tintColor;

void main()
{
    gl_Position = projection_matrx * model_matrx * vec4(aPos.xy, zDepth, 1.0);
    vertexColor = aColor * tintColor;
    texCoord = aTexCoord;
}
)";

    const string kSpriteFragmentShader = R"(#version 330 core
in vec4 vertexColor;
in vec2 texCoord;

out vec4 FragColor;

uniform sampler2D tex;

void main()
{
    FragColor = texture(tex, texCoord) * vertexColor;
}
)";

    string formatSeconds(double value)
    {
        const int seconds = static_cast<int>(value);
        const int minutesPart = seconds / 60;
        const int secondsPart = seconds % 60;
        ostringstream stream;
        stream << minutesPart << ":" << setw(2) << setfill('0') << secondsPart;
        return stream.str();
    }

    int findFriendlyResourceNodeIndex(const SandforgeWorld& world, SandforgeResourceType resourceType)
    {
        for (size_t index = 0; index < world.getNodes().size(); ++index)
        {
            const SandforgeResourceNode& node = world.getNodes()[index];
            if (node.ownerId != 1 || node.resourceType != resourceType)
            {
                continue;
            }

            return static_cast<int>(index);
        }

        return -1;
    }

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

void GameplayState::reset()
{
    _unitActors.clear();
    _buildingActors.clear();
    _nodeActors.clear();
    _terrainTiles.clear();
    _commandSlotBackgrounds.clear();
    _nodeStatusFrames.clear();
    _minimapBlips.clear();
    _topResourcePanel.reset();
    _infoPanel.reset();
    _rightSelectionPanel.reset();
    _minimapPanel.reset();
    _hudIcons.clear();
    _hudPanel.reset();
    _productionPanel.reset();
    _tooltipPanel.reset();
    _friendlySelectionRing.reset();
    _enemySelectionRing.reset();
    _rallyMarker.reset();
    _rallyGuideDot.reset();
    _queueProgressBar.reset();
    _queueProgressFrame.reset();
    _healthBarFrame.reset();
    _healthBarFill.reset();
    _buildPreviewSprite.reset();
    _attackEffectSprite.reset();
    _queuePreviewIcons.clear();
    _uiDecorSprites.clear();
    _portraitSprites.clear();
    _activePortrait.reset();
    _selectionKind = SandforgeSelectionKind::Barracks;
    _selectedNodeIndex = 0;
    _selectedUnitId = 0;
    _selectedBuildingId = 0;
    _buildPreviewKind = SandforgeBuildPreviewKind::None;
    _buildPreviewNodeIndex = -1;
    _hoveredCommandHotkey = 0;
    _cursorScreenPosition = vec2(0.0f, 0.0f);
    _cursorWorldPosition = vec2(0.0f, 0.0f);
    _elapsedTime = 0.0;
    _world.reset();
    _statusText = _world.getStatusText();
    initializeRenderer();
}

void GameplayState::setViewportSize(int width, int height)
{
    if (width <= 0 || height <= 0)
    {
        return;
    }

    _uiViewportSize = vec2(static_cast<float>(width), static_cast<float>(height));
}

void GameplayState::update(const InputState& input, double deltaTime, const vec2& cursorScreenPosition, const vec2& cursorWorldPosition)
{
    _elapsedTime += deltaTime;
    _cursorScreenPosition = cursorScreenPosition;
    _cursorWorldPosition = cursorWorldPosition;
    updateHoveredHudCommand(cursorScreenPosition);
    handlePlayerInput(input, cursorScreenPosition, cursorWorldPosition);
    _world.update(deltaTime);
    _statusText = _world.getStatusText();
}

void GameplayState::render(const mat4& worldProjection, const mat4& uiProjection, double deltaTime)
{
    initializeRenderer();
    initializeStaticArt();
    syncVisuals();

    if (_spriteShader == nullptr)
    {
        return;
    }

    _spriteShader->use();
    _spriteShader->setMat4("projection_matrx", worldProjection);
    _spriteShader->setInt("tex", 0);

    renderStaticArt();

    for (const SandforgeResourceNode& node : _world.getNodes())
    {
        const auto iterator = _nodeActors.find(node.id);
        if (iterator != _nodeActors.end() && iterator->second.sprite != nullptr)
        {
            iterator->second.sprite->SetDepth(-0.05f);
            iterator->second.sprite->Draw();
        }
    }

    for (const SandforgeBuilding& building : _world.getBuildings())
    {
        const auto iterator = _buildingActors.find(building.id);
        if (iterator != _buildingActors.end() && iterator->second.sprite != nullptr)
        {
            iterator->second.sprite->SetDepth(0.0f);
            iterator->second.sprite->Draw();
        }
    }

    for (const SandforgeUnit& unit : _world.getUnits())
    {
        const auto iterator = _unitActors.find(unit.id);
        if (iterator == _unitActors.end() || iterator->second.sprite == nullptr)
        {
            continue;
        }

        if (iterator->second.animation != nullptr)
        {
            iterator->second.sprite->ApplyAnimation(*iterator->second.animation, deltaTime);
        }

        iterator->second.sprite->SetDepth(0.05f);
        iterator->second.sprite->Draw();
    }

    renderFloatingHealthBars();
    renderEffects();

    _spriteShader->use();
    _spriteShader->setMat4("projection_matrx", uiProjection);
    _spriteShader->setInt("tex", 0);
    renderHudArt();
}

double GameplayState::getElapsedTime() const
{
    return _elapsedTime;
}

const string& GameplayState::getStatusText() const
{
    return _statusText;
}

vector<string> GameplayState::buildHudLines() const
{
    const SandforgePlayerState& player = _world.getPlayerState(1);
    const SandforgeMatchResult result = _world.getMatchResult();

    vector<string> lines;
    lines.push_back("Time " + formatSeconds(_world.getElapsedTime()) + "  Metal " + to_string(player.metal) + "  Energy " + to_string(player.energy));
    lines.push_back("Selection  " + buildSelectionLabel());
    lines.push_back("Status  " + _statusText);
    if (result.gameOver)
    {
        lines.push_back(result.winnerPlayerId == 1 ? "Result  Victory" : "Result  Defeat");
    }
    return lines;
}

string GameplayState::buildTopBarText() const
{
    const SandforgePlayerState& player = _world.getPlayerState(1);
    return "Time " + formatSeconds(_world.getElapsedTime()) +
        "   Metal " + to_string(player.metal) +
        "   Energy " + to_string(player.energy);
}

string GameplayState::buildSelectionTitle() const
{
    return buildSelectionLabel();
}

vector<string> GameplayState::buildSelectionSummary() const
{
    vector<string> details = buildSelectionDetails();
    if (details.size() > 3)
    {
        details.resize(3);
    }
    return details;
}

vector<string> GameplayState::buildCommandHints() const
{
    vector<string> lines;
    if (_selectionKind == SandforgeSelectionKind::HQ)
    {
        lines = { "1 Worker", "B Barracks", "F Factory" };
    }
    else if (_selectionKind == SandforgeSelectionKind::Barracks)
    {
        lines = { "2 Soldier", "3 Defender" };
    }
    else if (_selectionKind == SandforgeSelectionKind::Factory)
    {
        lines = { "6 Mech", "7 Siege" };
    }
    else if (_selectionKind == SandforgeSelectionKind::Node)
    {
        lines = { "4 Node Hub", "8 Tower" };
    }
    else if (_selectionKind == SandforgeSelectionKind::Unit)
    {
        const SandforgeUnit* unit = getSelectedUnit();
        if (unit != nullptr && unit->unitType == SandforgeUnitType::Worker)
        {
            lines = { "W Metal", "E Energy" };
        }
        else
        {
            lines = { "Right Click Move" };
        }
    }

    return lines;
}

bool GameplayState::isMatchOver() const
{
    return _world.getMatchResult().gameOver;
}

bool GameplayState::cancelSelectedProduction()
{
    if (_selectionKind == SandforgeSelectionKind::HQ)
    {
        return _world.cancelLastProduction(1, SandforgeBuildingType::HQ);
    }
    if (_selectionKind == SandforgeSelectionKind::Barracks)
    {
        if (_selectedBuildingId != 0)
        {
            return _world.cancelLastProduction(1, _selectedBuildingId);
        }
    }
    if (_selectionKind == SandforgeSelectionKind::Factory)
    {
        if (_selectedBuildingId != 0)
        {
            return _world.cancelLastProduction(1, _selectedBuildingId);
        }
    }

    _statusText = "Select a production building to cancel its last queue item.";
    return false;
}

bool GameplayState::hasCommandTooltip() const
{
    return _hoveredCommandHotkey != 0;
}

string GameplayState::buildCommandTooltipTitle() const
{
    switch (_hoveredCommandHotkey)
    {
    case GLFW_KEY_1: return "Worker";
    case GLFW_KEY_2: return "Soldier";
    case GLFW_KEY_3: return "Defender";
    case GLFW_KEY_4: return "Node Hub";
    case GLFW_KEY_B: return "Barracks";
    case GLFW_KEY_F: return "Factory";
    case GLFW_KEY_6: return "Ranger Mech";
    case GLFW_KEY_7: return "Siege Unit";
    case GLFW_KEY_8: return "Defense Tower";
    default: return "";
    }
}

vector<string> GameplayState::buildCommandTooltipLines() const
{
    vector<string> lines;

    auto addCostLines = [&](const string& title, int metal, int energy, const string& hint)
    {
        lines.push_back("Cost  " + to_string(metal) + "M  " + to_string(energy) + "E");
        lines.push_back(hint);
        lines.push_back("Click or hotkey: " + title);
    };

    switch (_hoveredCommandHotkey)
    {
    case GLFW_KEY_1:
    {
        const auto& unit = SandforgeDatabase::getUnit(SandforgeUnitType::Worker);
        addCostLines("1", unit.metalCost, unit.energyCost, "Gather metal and energy.");
        break;
    }
    case GLFW_KEY_2:
    {
        const auto& unit = SandforgeDatabase::getUnit(SandforgeUnitType::Soldier);
        addCostLines("2", unit.metalCost, unit.energyCost, "Basic frontline attacker.");
        break;
    }
    case GLFW_KEY_3:
    {
        const auto& unit = SandforgeDatabase::getUnit(SandforgeUnitType::Defender);
        addCostLines("3", unit.metalCost, unit.energyCost, "Anti-unit support trooper.");
        break;
    }
    case GLFW_KEY_4:
    {
        const auto& building = SandforgeDatabase::getBuilding(SandforgeBuildingType::NodeHub);
        addCostLines("4", building.metalCost, building.energyCost, "Boosts nearby resource income.");
        break;
    }
    case GLFW_KEY_B:
    {
        const auto& building = SandforgeDatabase::getBuilding(SandforgeBuildingType::Barracks);
        addCostLines("B", building.metalCost, building.energyCost, "Trains core combat units.");
        break;
    }
    case GLFW_KEY_F:
    {
        const auto& building = SandforgeDatabase::getBuilding(SandforgeBuildingType::Factory);
        addCostLines("F", building.metalCost, building.energyCost, "Unlocks mech production.");
        break;
    }
    case GLFW_KEY_6:
    {
        const auto& unit = SandforgeDatabase::getUnit(SandforgeUnitType::RangerMech);
        addCostLines("6", unit.metalCost, unit.energyCost, "Fast ranged mech.");
        break;
    }
    case GLFW_KEY_7:
    {
        const auto& unit = SandforgeDatabase::getUnit(SandforgeUnitType::SiegeUnit);
        addCostLines("7", unit.metalCost, unit.energyCost, "Heavy damage versus structures.");
        break;
    }
    case GLFW_KEY_8:
    {
        const auto& building = SandforgeDatabase::getBuilding(SandforgeBuildingType::DefenseTower);
        addCostLines("8", building.metalCost, building.energyCost, "Auto-attacks nearby enemies.");
        break;
    }
    default:
        break;
    }

    return lines;
}

vec2 GameplayState::getCommandTooltipPosition() const
{
    const float sx = _uiViewportSize.x / 1280.0f;
    const float sy = _uiViewportSize.y / 720.0f;
    const float uiScale = (std::min)(sx, sy);
    vec2 tooltipPosition = _cursorScreenPosition + vec2(18.0f, 22.0f);
    tooltipPosition.x = glm::clamp(tooltipPosition.x, 20.0f * uiScale, _uiViewportSize.x - (280.0f * uiScale));
    tooltipPosition.y = glm::clamp(tooltipPosition.y, 150.0f * uiScale, _uiViewportSize.y - (120.0f * uiScale));
    return tooltipPosition;
}

void GameplayState::initializeRenderer()
{
    if (_spriteShader != nullptr)
    {
        return;
    }

    _spriteShader = make_shared<Shader>(kSpriteVertexShader, kSpriteFragmentShader);
    _spriteShader->use();
    _spriteShader->setInt("tex", 0);
}

void GameplayState::initializeStaticArt()
{
    if (!_terrainTiles.empty())
    {
        return;
    }

    const array<string, 3> tilePaths =
    {
        "Assets/Tiles/sand_base.png",
        "Assets/Tiles/battlefield_grass.png",
        "Assets/Tiles/metal_plating.png"
    };

    for (int y = 0; y < 8; ++y)
    {
        for (int x = 0; x < 27; ++x)
        {
            const string& path = tilePaths[(x + y) % tilePaths.size()];
            _terrainTiles.push_back(createStaticSprite(path, vec2(static_cast<float>(x * 96), static_cast<float>(y * 96)), vec2(96.0f, 96.0f)));
        }
    }

    _topResourcePanel = createStaticSprite("Assets/UI/tooltip_panel.png", vec2(900.0f, 646.0f), vec2(356.0f, 50.0f));
    _hudPanel = createStaticSprite("Assets/UI/hud_panel.png", vec2(0.0f, 0.0f), vec2(1280.0f, 156.0f));
    _productionPanel = createStaticSprite("Assets/UI/production_panel.png", vec2(914.0f, 12.0f), vec2(340.0f, 132.0f));
    _infoPanel = createStaticSprite("Assets/UI/tooltip_panel.png", vec2(286.0f, 10.0f), vec2(208.0f, 132.0f));
    _tooltipPanel = createStaticSprite("Assets/UI/tooltip_panel.png", vec2(0.0f, 0.0f), vec2(270.0f, 112.0f));
    _rightSelectionPanel = createStaticSprite("Assets/UI/tooltip_panel.png", vec2(24.0f, 14.0f), vec2(250.0f, 126.0f));
    _minimapPanel = createStaticSprite("Assets/UI/hud_panel.png", vec2(506.0f, 12.0f), vec2(256.0f, 132.0f));
    _friendlySelectionRing = createStaticSprite("Assets/Effects/selection_ring_friendly.png", vec2(0.0f, 0.0f), vec2(92.0f, 92.0f));
    _enemySelectionRing = createStaticSprite("Assets/Effects/selection_ring_enemy.png", vec2(0.0f, 0.0f), vec2(108.0f, 108.0f));
    _rallyMarker = createStaticSprite("Assets/Effects/rally_marker.png", vec2(0.0f, 0.0f), vec2(42.0f, 42.0f));
    _rallyGuideDot = createStaticSprite("Assets/Effects/rally_marker.png", vec2(0.0f, 0.0f), vec2(18.0f, 18.0f));
    _queueProgressBar = createStaticSprite("Assets/Tiles/metal_plating.png", vec2(0.0f, 0.0f), vec2(1.0f, 8.0f));
    _queueProgressFrame = createStaticSprite("Assets/Tiles/sand_base.png", vec2(0.0f, 0.0f), vec2(1.0f, 8.0f));
    _buildPreviewSprite = createStaticSprite("Assets/Buildings/barracks.png", vec2(0.0f, 0.0f), vec2(96.0f, 96.0f));
    _attackEffectSprite = createStaticSprite("Assets/Effects/rally_marker.png", vec2(0.0f, 0.0f), vec2(14.0f, 14.0f));
    _healthBarFrame = createStaticSprite("Assets/Tiles/sand_base.png", vec2(0.0f, 0.0f), vec2(120.0f, 10.0f));
    _healthBarFill = createStaticSprite("Assets/Tiles/metal_plating.png", vec2(0.0f, 0.0f), vec2(120.0f, 10.0f));

    const array<string, 6> iconPaths =
    {
        "Assets/UI/Icons/icon_worker.png",
        "Assets/UI/Icons/icon_soldier.png",
        "Assets/UI/Icons/icon_defender.png",
        "Assets/UI/Icons/icon_build.png",
        "Assets/UI/Icons/icon_factory.png",
        "Assets/UI/Icons/icon_nodehub.png"
    };

    for (size_t index = 0; index < iconPaths.size(); ++index)
    {
        _hudIcons.push_back(createStaticSprite(iconPaths[index], vec2(0.0f, 0.0f), vec2(40.0f, 40.0f)));
    }

    for (int index = 0; index < 6; ++index)
    {
        _commandSlotBackgrounds.push_back(createStaticSprite("Assets/UI/tooltip_panel.png", vec2(0.0f, 0.0f), vec2(64.0f, 50.0f)));
    }

    for (int index = 0; index < 2; ++index)
    {
        _nodeStatusFrames.push_back(createStaticSprite("Assets/UI/tooltip_panel.png", vec2(304.0f + (index * 56.0f), 76.0f), vec2(50.0f, 38.0f)));
    }

    for (int index = 0; index < 6; ++index)
    {
        _minimapBlips.push_back(createStaticSprite("Assets/Effects/rally_marker.png", vec2(0.0f, 0.0f), vec2(18.0f, 18.0f)));
    }

    _portraitSprites["hq"] = createStaticSprite("Assets/UI/Portraits/portrait_hq.png", vec2(1000.0f, 20.0f), vec2(92.0f, 92.0f));
    _portraitSprites["barracks"] = createStaticSprite("Assets/UI/Portraits/portrait_barracks.png", vec2(1000.0f, 20.0f), vec2(92.0f, 92.0f));
    _portraitSprites["worker"] = createStaticSprite("Assets/UI/Portraits/portrait_worker.png", vec2(1000.0f, 20.0f), vec2(92.0f, 92.0f));
    _portraitSprites["soldier"] = createStaticSprite("Assets/UI/Portraits/portrait_soldier.png", vec2(1000.0f, 20.0f), vec2(92.0f, 92.0f));
    _portraitSprites["defender"] = createStaticSprite("Assets/UI/Portraits/portrait_defender.png", vec2(1000.0f, 20.0f), vec2(92.0f, 92.0f));
    _portraitSprites["ranger_mech"] = createStaticSprite("Assets/UI/Portraits/portrait_ranger_mech.png", vec2(1000.0f, 20.0f), vec2(92.0f, 92.0f));

    _queuePreviewIcons[SandforgeUnitType::Worker] = createStaticSprite("Assets/UI/Icons/icon_worker.png", vec2(0.0f, 0.0f), vec2(44.0f, 44.0f));
    _queuePreviewIcons[SandforgeUnitType::Soldier] = createStaticSprite("Assets/UI/Icons/icon_soldier.png", vec2(0.0f, 0.0f), vec2(44.0f, 44.0f));
    _queuePreviewIcons[SandforgeUnitType::Defender] = createStaticSprite("Assets/UI/Icons/icon_defender.png", vec2(0.0f, 0.0f), vec2(44.0f, 44.0f));
    _queuePreviewIcons[SandforgeUnitType::RangerMech] = createStaticSprite("Assets/UI/Icons/icon_factory.png", vec2(0.0f, 0.0f), vec2(44.0f, 44.0f));
    _queuePreviewIcons[SandforgeUnitType::SiegeUnit] = createStaticSprite("Assets/UI/Icons/icon_build.png", vec2(0.0f, 0.0f), vec2(44.0f, 44.0f));
    _uiDecorSprites["factory"] = createStaticSprite("Assets/UI/Icons/icon_factory.png", vec2(0.0f, 0.0f), vec2(44.0f, 44.0f));
    _uiDecorSprites["barracks"] = createStaticSprite("Assets/Buildings/barracks.png", vec2(0.0f, 0.0f), vec2(44.0f, 44.0f));
    _uiDecorSprites["nodehub"] = createStaticSprite("Assets/UI/Icons/icon_nodehub.png", vec2(0.0f, 0.0f), vec2(44.0f, 44.0f));
    _uiDecorSprites["tower"] = createStaticSprite("Assets/UI/Icons/icon_defender.png", vec2(0.0f, 0.0f), vec2(44.0f, 44.0f));
    _uiDecorSprites["metal_node"] = createStaticSprite("Assets/Nodes/metal_node.png", vec2(0.0f, 0.0f), vec2(50.0f, 50.0f));
    _uiDecorSprites["energy_node"] = createStaticSprite("Assets/Nodes/energy_core.png", vec2(0.0f, 0.0f), vec2(50.0f, 50.0f));
}

void GameplayState::syncVisuals()
{
    unordered_set<SandforgeEntityId> liveIds;

    for (const SandforgeResourceNode& node : _world.getNodes())
    {
        const SandforgeVisualDefinition& visual = SandforgeDatabase::getNode(node.resourceType).visuals;
        updateVisualActor(_nodeActors, node.id, visual, node.position, visual.spriteSize);
        liveIds.insert(node.id);
    }
    for (auto iterator = _nodeActors.begin(); iterator != _nodeActors.end();)
    {
        if (!liveIds.contains(iterator->first)) iterator = _nodeActors.erase(iterator);
        else ++iterator;
    }

    liveIds.clear();
    for (const SandforgeBuilding& building : _world.getBuildings())
    {
        const SandforgeVisualDefinition& visual = SandforgeDatabase::getBuilding(building.buildingType).visuals;
        updateVisualActor(_buildingActors, building.id, visual, building.position, visual.spriteSize);
        liveIds.insert(building.id);
    }
    for (auto iterator = _buildingActors.begin(); iterator != _buildingActors.end();)
    {
        if (!liveIds.contains(iterator->first)) iterator = _buildingActors.erase(iterator);
        else ++iterator;
    }

    liveIds.clear();
    for (const SandforgeUnit& unit : _world.getUnits())
    {
        const SandforgeVisualDefinition& visual = SandforgeDatabase::getUnit(unit.unitType).visuals;
        updateVisualActor(_unitActors, unit.id, visual, unit.position, visual.spriteSize);
        liveIds.insert(unit.id);
    }
    for (auto iterator = _unitActors.begin(); iterator != _unitActors.end();)
    {
        if (!liveIds.contains(iterator->first)) iterator = _unitActors.erase(iterator);
        else ++iterator;
    }
}

void GameplayState::updateVisualActor(unordered_map<SandforgeEntityId, SandforgeVisualActor>& actors, SandforgeEntityId id, const SandforgeVisualDefinition& visualDefinition, const SandforgeVec2& position, const vec2& size)
{
    auto& actor = actors[id];
    if (actor.sprite == nullptr)
    {
        actor.sprite = make_shared<Sprite>(_spriteShader, visualDefinition.imagePath.c_str());
        actor.sprite->SetScale(size);

        if (!visualDefinition.animationPath.empty() && filesystem::exists(visualDefinition.animationPath))
        {
            actor.animation = make_unique<Animation2D>(visualDefinition.animationPath.c_str());
        }
    }

    actor.sprite->SetPosition(position.x - (size.x * 0.5f), position.y - (size.y * 0.5f));
}

shared_ptr<Sprite> GameplayState::createStaticSprite(const string& imagePath, const vec2& position, const vec2& size)
{
    shared_ptr<Sprite> sprite = make_shared<Sprite>(_spriteShader, imagePath.c_str());
    sprite->SetPosition(position);
    sprite->SetScale(size);
    return sprite;
}

void GameplayState::renderStaticArt()
{
    for (const auto& tile : _terrainTiles)
    {
        if (tile == nullptr) continue;
        tile->SetDepth(-0.2f);
        tile->Draw();
    }
}

void GameplayState::renderEffects()
{
    if (_friendlySelectionRing == nullptr)
    {
        return;
    }

    if (_selectionKind == SandforgeSelectionKind::Unit)
    {
        const SandforgeUnit* unit = getSelectedUnit();
        if (unit != nullptr)
        {
            const vec2 size = SandforgeDatabase::getUnit(unit->unitType).visuals.spriteSize;
            _friendlySelectionRing->SetPosition(unit->position.x - (size.x * 0.7f), unit->position.y - (size.y * 0.7f));
            _friendlySelectionRing->SetScale(size * 1.4f);
            _friendlySelectionRing->SetDepth(-0.03f);
            _friendlySelectionRing->Draw();

            if (_rallyGuideDot != nullptr)
            {
                _rallyGuideDot->SetPosition(unit->moveTarget.x - 9.0f, unit->moveTarget.y - 9.0f);
                _rallyGuideDot->SetDepth(0.09f);
                _rallyGuideDot->Draw();
            }
        }
    }

    if (_selectionKind == SandforgeSelectionKind::Node && _selectedNodeIndex >= 0 && _selectedNodeIndex < static_cast<int>(_world.getNodes().size()))
    {
        const SandforgeResourceNode& node = _world.getNodes()[_selectedNodeIndex];
        _friendlySelectionRing->SetPosition(node.position.x - 46.0f, node.position.y - 46.0f);
        _friendlySelectionRing->SetScale(vec2(92.0f, 92.0f));
        _friendlySelectionRing->SetDepth(-0.03f);
        _friendlySelectionRing->Draw();
    }

    if (_selectionKind == SandforgeSelectionKind::HQ)
    {
        const SandforgeBuilding* hq = _world.findPrimaryBuilding(1, SandforgeBuildingType::HQ);
        if (hq != nullptr)
        {
            _friendlySelectionRing->SetPosition(hq->position.x - 58.0f, hq->position.y - 58.0f);
            _friendlySelectionRing->SetScale(vec2(116.0f, 116.0f));
            _friendlySelectionRing->SetDepth(-0.03f);
            _friendlySelectionRing->Draw();
        }
    }

    if (_selectionKind == SandforgeSelectionKind::Barracks)
    {
        const SandforgeBuilding* barracks = getSelectedBuilding();
        if (barracks != nullptr)
        {
            _friendlySelectionRing->SetPosition(barracks->position.x - 50.0f, barracks->position.y - 50.0f);
            _friendlySelectionRing->SetScale(vec2(100.0f, 100.0f));
            _friendlySelectionRing->SetDepth(-0.03f);
            _friendlySelectionRing->Draw();
        }
    }

    if (_selectionKind == SandforgeSelectionKind::Factory)
    {
        const SandforgeBuilding* factory = getSelectedBuilding();
        if (factory != nullptr)
        {
            _friendlySelectionRing->SetPosition(factory->position.x - 54.0f, factory->position.y - 54.0f);
            _friendlySelectionRing->SetScale(vec2(108.0f, 108.0f));
            _friendlySelectionRing->SetDepth(-0.03f);
            _friendlySelectionRing->Draw();
        }
    }

    if (_selectionKind == SandforgeSelectionKind::NodeHub)
    {
        for (const SandforgeBuilding& building : _world.getBuildings())
        {
            if (!building.alive || building.ownerId != 1 || building.buildingType != SandforgeBuildingType::NodeHub)
            {
                continue;
            }

            _friendlySelectionRing->SetPosition(building.position.x - 46.0f, building.position.y - 46.0f);
            _friendlySelectionRing->SetScale(vec2(92.0f, 92.0f));
            _friendlySelectionRing->SetDepth(-0.03f);
            _friendlySelectionRing->Draw();
            break;
        }
    }

    if (_selectionKind == SandforgeSelectionKind::DefenseTower)
    {
        for (const SandforgeBuilding& building : _world.getBuildings())
        {
            if (!building.alive || building.ownerId != 1 || building.buildingType != SandforgeBuildingType::DefenseTower)
            {
                continue;
            }

            _friendlySelectionRing->SetPosition(building.position.x - 44.0f, building.position.y - 44.0f);
            _friendlySelectionRing->SetScale(vec2(88.0f, 88.0f));
            _friendlySelectionRing->SetDepth(-0.03f);
            _friendlySelectionRing->Draw();
            break;
        }
    }

    const SandforgeBuilding* enemyHQ = _world.findPrimaryBuilding(2, SandforgeBuildingType::HQ);
    if (enemyHQ != nullptr && _enemySelectionRing != nullptr)
    {
        _enemySelectionRing->SetPosition(enemyHQ->position.x - 54.0f, enemyHQ->position.y - 54.0f);
        _enemySelectionRing->SetDepth(-0.02f);
        _enemySelectionRing->Draw();
    }

    for (const SandforgeCombatEffect& effect : _world.getCombatEffects())
    {
        if (_attackEffectSprite == nullptr)
        {
            break;
        }

        const vec2 start(effect.from.x, effect.from.y);
        const vec2 end(effect.to.x, effect.to.y);
        const vec2 delta = end - start;
        const float distance = glm::length(delta);
        if (distance <= 1.0f)
        {
            continue;
        }

        const vec2 direction = delta / distance;
        const int markerCount = (std::min)(4, (std::max)(2, static_cast<int>(distance / 48.0f)));
        for (int index = 0; index < markerCount; ++index)
        {
            const float t = static_cast<float>(index + 1) / static_cast<float>(markerCount + 1);
            const vec2 point = start + (direction * distance * t);
            _attackEffectSprite->SetPosition(point.x - 7.0f, point.y - 7.0f);
            _attackEffectSprite->SetDepth(0.10f);
            _attackEffectSprite->Draw();
        }
    }

    for (const SandforgeResourceNode& node : _world.getNodes())
    {
        if (node.harvestingWorkerId == 0 || _rallyGuideDot == nullptr)
        {
            continue;
        }

        const float pulse = static_cast<float>(fmod(_world.getElapsedTime() * 2.0 + static_cast<double>(node.id), 1.0));
        for (int index = 0; index < 3; ++index)
        {
            const float angle = pulse + (static_cast<float>(index) * 0.33f);
            const float offsetX = cos(angle * glm::two_pi<float>()) * 18.0f;
            const float offsetY = sin(angle * glm::two_pi<float>()) * 10.0f;
            _rallyGuideDot->SetPosition(node.position.x + offsetX - 6.0f, node.position.y + offsetY + 20.0f);
            _rallyGuideDot->SetScale(vec2(12.0f, 12.0f));
            _rallyGuideDot->SetDepth(0.095f);
            _rallyGuideDot->Draw();
        }
    }

    if (_buildPreviewKind != SandforgeBuildPreviewKind::None && _buildPreviewSprite != nullptr)
    {
        const SandforgeBuildingType previewType = getPreviewBuildingType();
        const vec2 previewSize = SandforgeDatabase::getBuilding(previewType).visuals.spriteSize;
        _buildPreviewSprite->SetScale(previewSize);
        _buildPreviewSprite->SetDepth(0.02f);
        _buildPreviewSprite->Draw();

        const vec2 previewCenter = _buildPreviewSprite->GetPosition() + (previewSize * 0.5f);
        const bool valid = isBuildPreviewValid(previewCenter);
        shared_ptr<Sprite> ring = valid ? _friendlySelectionRing : _enemySelectionRing;
        if (ring != nullptr)
        {
            ring->SetPosition(previewCenter.x - (previewSize.x * 0.65f), previewCenter.y - (previewSize.y * 0.65f));
            ring->SetScale(previewSize * 1.3f);
            ring->SetDepth(-0.01f);
            ring->Draw();
        }
    }
}

void GameplayState::renderHudArt()
{
    gHudRenderer.renderHudArt(*this);
}

void GameplayState::renderFloatingHealthBars()
{
    if (_healthBarFrame == nullptr || _healthBarFill == nullptr)
    {
        return;
    }

    for (const SandforgeBuilding& building : _world.getBuildings())
    {
        if (!building.alive || building.maxHp <= 0.0f)
        {
            continue;
        }

        const vec2 size = SandforgeDatabase::getBuilding(building.buildingType).visuals.spriteSize;
        const float width = size.x * 0.7f;
        const float ratio = glm::clamp(building.hp / building.maxHp, 0.0f, 1.0f);
        const vec2 position(building.position.x - (width * 0.5f), building.position.y + (size.y * 0.48f));

        _healthBarFrame->SetTintColor(0.18f, 0.05f, 0.05f, 1.0f);
        _healthBarFrame->SetPosition(position);
        _healthBarFrame->SetScale(vec2(width, 7.0f));
        _healthBarFrame->SetDepth(0.11f);
        _healthBarFrame->Draw();

        _healthBarFill->SetTintColor(0.88f, 0.14f, 0.14f, 1.0f);
        _healthBarFill->SetPosition(position);
        _healthBarFill->SetScale(vec2(width * ratio, 7.0f));
        _healthBarFill->SetDepth(0.115f);
        _healthBarFill->Draw();
    }

    for (const SandforgeUnit& unit : _world.getUnits())
    {
        if (!unit.alive || unit.maxHp <= 0.0f)
        {
            continue;
        }

        const vec2 size = SandforgeDatabase::getUnit(unit.unitType).visuals.spriteSize;
        const float width = size.x * 0.8f;
        const float ratio = glm::clamp(unit.hp / unit.maxHp, 0.0f, 1.0f);
        const vec2 position(unit.position.x - (width * 0.5f), unit.position.y + (size.y * 0.52f));

        _healthBarFrame->SetTintColor(0.18f, 0.05f, 0.05f, 1.0f);
        _healthBarFrame->SetPosition(position);
        _healthBarFrame->SetScale(vec2(width, 5.0f));
        _healthBarFrame->SetDepth(0.11f);
        _healthBarFrame->Draw();

        _healthBarFill->SetTintColor(0.88f, 0.14f, 0.14f, 1.0f);
        _healthBarFill->SetPosition(position);
        _healthBarFill->SetScale(vec2(width * ratio, 5.0f));
        _healthBarFill->SetDepth(0.115f);
        _healthBarFill->Draw();
    }
}

vector<GameplayState::HudCommandButton> GameplayState::buildHudCommandButtons() const
{
    return gHudRenderer.buildHudCommandButtons(*this);
}

void GameplayState::updateHoveredHudCommand(const vec2& cursorScreenPosition)
{
    gHudRenderer.updateHoveredHudCommand(*this, cursorScreenPosition);
}

bool GameplayState::handleHudClick(const vec2& cursorScreenPosition, const vec2& cursorWorldPosition)
{
    return gHudRenderer.handleHudClick(*this, cursorScreenPosition, cursorWorldPosition);
}

bool GameplayState::activateHudCommand(int hotkey, const vec2& cursorWorldPosition)
{
    return gHudRenderer.activateHudCommand(*this, hotkey, cursorWorldPosition);
}

void GameplayState::setSelection(SandforgeSelectionKind kind, int index)
{
    gSelectionController.setSelection(*this, kind, index);
}

const SandforgeBuilding* GameplayState::getSelectedBuilding() const
{
    return gSelectionController.getSelectedBuilding(*this);
}

void GameplayState::setBuildingSelection(SandforgeSelectionKind kind, SandforgeEntityId buildingId)
{
    gSelectionController.setBuildingSelection(*this, kind, buildingId);
}

void GameplayState::beginBuildPreview(SandforgeBuildPreviewKind kind, const vec2& cursorWorldPosition)
{
    gBuildPreviewController.beginBuildPreview(*this, kind, cursorWorldPosition);
}

void GameplayState::cancelBuildPreview()
{
    gBuildPreviewController.cancelBuildPreview(*this);
}

bool GameplayState::tryPlacePreviewAt(const vec2& cursorWorldPosition)
{
    return gBuildPreviewController.tryPlacePreviewAt(*this, cursorWorldPosition);
}

bool GameplayState::isBuildPreviewValid(const vec2& cursorWorldPosition) const
{
    return gBuildPreviewController.isBuildPreviewValid(*this, cursorWorldPosition);
}

SandforgeBuildingType GameplayState::getPreviewBuildingType() const
{
    return gBuildPreviewController.getPreviewBuildingType(*this);
}

const SandforgeUnit* GameplayState::getSelectedUnit() const
{
    return gSelectionController.getSelectedUnit(*this);
}

string GameplayState::buildSelectionLabel() const
{
    return gSelectionController.buildSelectionLabel(*this);
}

vector<string> GameplayState::buildSelectionDetails() const
{
    return gSelectionController.buildSelectionDetails(*this);
}

void GameplayState::handlePlayerInput(const InputState& input, const vec2& cursorScreenPosition, const vec2& cursorWorldPosition)
{
    if (input.wasKeyPressed(GLFW_KEY_R))
    {
        reset();
        return;
    }

    if (_buildPreviewKind != SandforgeBuildPreviewKind::None && _buildPreviewSprite != nullptr)
    {
        const auto& definition = SandforgeDatabase::getBuilding(getPreviewBuildingType());
        _buildPreviewSprite->SetPosition(cursorWorldPosition - (definition.visuals.spriteSize * 0.5f));
    }

    if (_world.getMatchResult().gameOver)
    {
        return;
    }

    if (input.wasMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT))
    {
        if (handleHudClick(cursorScreenPosition, cursorWorldPosition))
        {
        }
        else if (_buildPreviewKind != SandforgeBuildPreviewKind::None)
        {
            tryPlacePreviewAt(cursorWorldPosition);
        }
        else
        {
            selectObjectAt(cursorWorldPosition);
        }
    }
    if (input.wasMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT))
    {
        if (_buildPreviewKind != SandforgeBuildPreviewKind::None)
        {
            cancelBuildPreview();
        }
        else if (_selectionKind == SandforgeSelectionKind::Unit)
        {
            _world.moveUnitTo(1, _selectedUnitId, sandforgeFromGlm(cursorWorldPosition));
        }
    }

    if (input.wasKeyPressed(GLFW_KEY_1))
    {
        if (_selectionKind == SandforgeSelectionKind::HQ)
        {
            _world.queueProduction(1, SandforgeBuildingType::HQ, SandforgeUnitType::Worker);
        }
        else
        {
            _statusText = "Select Headquarters first.";
        }
    }
    if (input.wasKeyPressed(GLFW_KEY_2))
    {
        if (_selectionKind == SandforgeSelectionKind::Barracks && _selectedBuildingId != 0)
        {
            _world.queueProduction(1, _selectedBuildingId, SandforgeUnitType::Soldier);
        }
        else
        {
            _statusText = "Select a Barracks first.";
        }
    }
    if (input.wasKeyPressed(GLFW_KEY_3))
    {
        if (_selectionKind == SandforgeSelectionKind::Barracks && _selectedBuildingId != 0)
        {
            _world.queueProduction(1, _selectedBuildingId, SandforgeUnitType::Defender);
        }
        else
        {
            _statusText = "Select a Barracks first.";
        }
    }
    if (input.wasKeyPressed(GLFW_KEY_6))
    {
        if (_selectionKind == SandforgeSelectionKind::Factory && _selectedBuildingId != 0)
        {
            _world.queueProduction(1, _selectedBuildingId, SandforgeUnitType::RangerMech);
        }
        else
        {
            _statusText = "Select a Factory first.";
        }
    }
    if (input.wasKeyPressed(GLFW_KEY_7))
    {
        if (_selectionKind == SandforgeSelectionKind::Factory && _selectedBuildingId != 0)
        {
            _world.queueProduction(1, _selectedBuildingId, SandforgeUnitType::SiegeUnit);
        }
        else
        {
            _statusText = "Select a Factory first.";
        }
    }
    if (input.wasKeyPressed(GLFW_KEY_F1))
    {
        _buildPreviewKind = SandforgeBuildPreviewKind::None;
        setSelection(SandforgeSelectionKind::HQ);
    }
    if (input.wasKeyPressed(GLFW_KEY_F2))
    {
        _buildPreviewKind = SandforgeBuildPreviewKind::None;
        for (const SandforgeBuilding& building : _world.getBuildings())
        {
            if (building.alive && building.ownerId == 1 && building.buildingType == SandforgeBuildingType::Barracks)
            {
                setBuildingSelection(SandforgeSelectionKind::Barracks, building.id);
                break;
            }
        }
    }
    if (input.wasKeyPressed(GLFW_KEY_F3))
    {
        _buildPreviewKind = SandforgeBuildPreviewKind::None;
        for (const SandforgeBuilding& building : _world.getBuildings())
        {
            if (building.alive && building.ownerId == 1 && building.buildingType == SandforgeBuildingType::Factory)
            {
                setBuildingSelection(SandforgeSelectionKind::Factory, building.id);
                break;
            }
        }
    }
    if (input.wasKeyPressed(GLFW_KEY_TAB))
    {
        if (_selectionKind == SandforgeSelectionKind::HQ)
        {
            for (const SandforgeBuilding& building : _world.getBuildings())
            {
                if (building.alive && building.ownerId == 1 && building.buildingType == SandforgeBuildingType::Barracks)
                {
                    setBuildingSelection(SandforgeSelectionKind::Barracks, building.id);
                    break;
                }
            }
        }
        else if (_selectionKind == SandforgeSelectionKind::Barracks)
        {
            bool advancedToNextBarracks = false;
            bool foundCurrent = false;
            for (const SandforgeBuilding& building : _world.getBuildings())
            {
                if (!building.alive || building.ownerId != 1 || building.buildingType != SandforgeBuildingType::Barracks)
                {
                    continue;
                }

                if (foundCurrent)
                {
                    setBuildingSelection(SandforgeSelectionKind::Barracks, building.id);
                    advancedToNextBarracks = true;
                    break;
                }

                if (building.id == _selectedBuildingId)
                {
                    foundCurrent = true;
                }
            }

            if (!advancedToNextBarracks)
            {
                for (const SandforgeBuilding& building : _world.getBuildings())
                {
                    if (building.alive && building.ownerId == 1 && building.buildingType == SandforgeBuildingType::Factory)
                    {
                        setBuildingSelection(SandforgeSelectionKind::Factory, building.id);
                        advancedToNextBarracks = true;
                        break;
                    }
                }
            }

            if (!advancedToNextBarracks)
            {
                const int nodeCount = static_cast<int>(_world.getNodes().size());
                if (nodeCount > 0) setSelection(SandforgeSelectionKind::Node, (_selectedNodeIndex + 1) % nodeCount);
                else setSelection(SandforgeSelectionKind::HQ);
            }
        }
        else if (_selectionKind == SandforgeSelectionKind::Factory)
        {
            const int nodeCount = static_cast<int>(_world.getNodes().size());
            if (nodeCount > 0) setSelection(SandforgeSelectionKind::Node, (_selectedNodeIndex + 1) % nodeCount);
        }
        else setSelection(SandforgeSelectionKind::HQ);
    }
    if (input.wasKeyPressed(GLFW_KEY_W))
    {
        const int nodeIndex = findFriendlyResourceNodeIndex(_world, SandforgeResourceType::Metal);
        if (nodeIndex >= 0)
        {
            const SandforgeEntityId selectedWorkerId = _selectedUnitId;
            const SandforgeUnit* selectedUnit = _world.findUnitById(selectedWorkerId);
            if (selectedUnit != nullptr && selectedUnit->unitType == SandforgeUnitType::Worker)
            {
                _world.assignWorkerToNode(1, selectedUnit->id, static_cast<size_t>(nodeIndex));
                setSelection(SandforgeSelectionKind::Unit);
                _selectedUnitId = selectedUnit->id;
            }
            else
            {
                _world.assignWorkerToNode(1, static_cast<size_t>(nodeIndex));
                setSelection(SandforgeSelectionKind::Node, nodeIndex);
            }
        }
    }
    if (input.wasKeyPressed(GLFW_KEY_E))
    {
        const int nodeIndex = findFriendlyResourceNodeIndex(_world, SandforgeResourceType::Energy);
        if (nodeIndex >= 0)
        {
            const SandforgeEntityId selectedWorkerId = _selectedUnitId;
            const SandforgeUnit* selectedUnit = _world.findUnitById(selectedWorkerId);
            if (selectedUnit != nullptr && selectedUnit->unitType == SandforgeUnitType::Worker)
            {
                _world.assignWorkerToNode(1, selectedUnit->id, static_cast<size_t>(nodeIndex));
                setSelection(SandforgeSelectionKind::Unit);
                _selectedUnitId = selectedUnit->id;
            }
            else
            {
                _world.assignWorkerToNode(1, static_cast<size_t>(nodeIndex));
                setSelection(SandforgeSelectionKind::Node, nodeIndex);
            }
        }
    }
    if (input.wasKeyPressed(GLFW_KEY_4) &&
        _selectionKind == SandforgeSelectionKind::Node &&
        _selectedNodeIndex >= 0 &&
        _selectedNodeIndex < static_cast<int>(_world.getNodes().size()))
    {
        beginBuildPreview(SandforgeBuildPreviewKind::NodeHub, cursorWorldPosition);
    }
    if (input.wasKeyPressed(GLFW_KEY_B) && _selectionKind == SandforgeSelectionKind::HQ)
    {
        beginBuildPreview(SandforgeBuildPreviewKind::Barracks, cursorWorldPosition);
    }
    if (input.wasKeyPressed(GLFW_KEY_F) && _selectionKind == SandforgeSelectionKind::HQ)
    {
        beginBuildPreview(SandforgeBuildPreviewKind::Factory, cursorWorldPosition);
    }
    if (input.wasKeyPressed(GLFW_KEY_8) &&
        _selectionKind == SandforgeSelectionKind::Node &&
        _selectedNodeIndex >= 0 &&
        _selectedNodeIndex < static_cast<int>(_world.getNodes().size()))
    {
        beginBuildPreview(SandforgeBuildPreviewKind::DefenseTower, cursorWorldPosition);
    }
}

bool GameplayState::selectObjectAt(const vec2& cursorWorldPosition)
{
    return gSelectionController.selectObjectAt(*this, cursorWorldPosition);
}
