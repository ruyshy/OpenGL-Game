#include "pch.h"
#include "GameplayHudRenderer.h"

#include "GameplayBuildPreviewController.h"
#include "GameplaySelectionController.h"
#include "Sprite.h"

void GameplayHudRenderer::renderHudArt(GameplayState& state) const
{
    const float sx = state._uiViewportSize.x / 1280.0f;
    const float sy = state._uiViewportSize.y / 720.0f;
    const float uiScale = (std::min)(sx, sy);
    const auto scalePos = [&](float x, float y) { return vec2(x * sx, y * sy); };
    const auto scaleSize = [&](float x, float y) { return vec2(x * sx, y * sy); };
    const auto uniformSize = [&](float x, float y) { return vec2(x * uiScale, y * uiScale); };

    if (state._topResourcePanel != nullptr)
    {
        state._topResourcePanel->SetDepth(0.145f);
        state._topResourcePanel->SetPosition(scalePos(900.0f, 646.0f));
        state._topResourcePanel->SetScale(scaleSize(356.0f, 50.0f));
        state._topResourcePanel->Draw();
    }
    if (state._hudPanel != nullptr)
    {
        state._hudPanel->SetDepth(0.15f);
        state._hudPanel->SetPosition(vec2(0.0f, 0.0f));
        state._hudPanel->SetScale(scaleSize(1280.0f, 156.0f));
        state._hudPanel->Draw();
    }
    if (state._productionPanel != nullptr)
    {
        state._productionPanel->SetDepth(0.15f);
        state._productionPanel->SetPosition(scalePos(914.0f, 12.0f));
        state._productionPanel->SetScale(scaleSize(340.0f, 132.0f));
        state._productionPanel->Draw();
    }
    if (state._rightSelectionPanel != nullptr)
    {
        state._rightSelectionPanel->SetDepth(0.15f);
        state._rightSelectionPanel->SetPosition(scalePos(24.0f, 14.0f));
        state._rightSelectionPanel->SetScale(scaleSize(250.0f, 126.0f));
        state._rightSelectionPanel->Draw();
    }
    if (state._infoPanel != nullptr)
    {
        state._infoPanel->SetDepth(0.15f);
        state._infoPanel->SetPosition(scalePos(286.0f, 10.0f));
        state._infoPanel->SetScale(scaleSize(208.0f, 132.0f));
        state._infoPanel->Draw();
    }
    if (state._minimapPanel != nullptr)
    {
        state._minimapPanel->SetDepth(0.15f);
        state._minimapPanel->SetPosition(scalePos(506.0f, 12.0f));
        state._minimapPanel->SetScale(scaleSize(256.0f, 132.0f));
        state._minimapPanel->Draw();
    }

    for (const auto& frame : state._commandSlotBackgrounds)
    {
        if (frame == nullptr) continue;
        frame->SetDepth(0.155f);
        frame->Draw();
    }

    const auto& nodes = state._world.getNodes();
    for (size_t index = 0; index < state._nodeStatusFrames.size() && index < nodes.size(); ++index)
    {
        if (state._nodeStatusFrames[index] == nullptr) continue;
        state._nodeStatusFrames[index]->SetPosition(scalePos(300.0f + (static_cast<float>(index) * 58.0f), 88.0f));
        state._nodeStatusFrames[index]->SetScale(scaleSize(50.0f, 38.0f));
        state._nodeStatusFrames[index]->SetDepth(0.155f);
        state._nodeStatusFrames[index]->Draw();
    }

    if (state._selectionKind == SandforgeSelectionKind::Unit)
    {
        const SandforgeUnit* unit = state.getSelectedUnit();
        if (unit != nullptr)
        {
            switch (unit->unitType)
            {
            case SandforgeUnitType::Worker: state._activePortrait = state._portraitSprites["worker"]; break;
            case SandforgeUnitType::Soldier: state._activePortrait = state._portraitSprites["soldier"]; break;
            case SandforgeUnitType::Defender: state._activePortrait = state._portraitSprites["defender"]; break;
            default: state._activePortrait = state._portraitSprites["ranger_mech"]; break;
            }
        }
    }
    else if (state._selectionKind == SandforgeSelectionKind::HQ)
    {
        state._activePortrait = state._portraitSprites["hq"];
    }
    else if (state._selectionKind == SandforgeSelectionKind::Barracks)
    {
        state._activePortrait = state._portraitSprites["barracks"];
    }
    else if (state._selectionKind == SandforgeSelectionKind::Factory)
    {
        state._activePortrait = state._portraitSprites["ranger_mech"];
    }
    else if (state._selectionKind == SandforgeSelectionKind::NodeHub)
    {
        state._activePortrait = state._portraitSprites["worker"];
    }
    else if (state._selectionKind == SandforgeSelectionKind::DefenseTower)
    {
        state._activePortrait = state._portraitSprites["defender"];
    }
    else if (state._selectionKind == SandforgeSelectionKind::Node)
    {
        const SandforgeResourceNode& node = state._world.getNodes()[state._selectedNodeIndex];
        state._activePortrait = node.resourceType == SandforgeResourceType::Metal ? state._portraitSprites["worker"] : state._portraitSprites["ranger_mech"];
    }

    if (state._activePortrait != nullptr)
    {
        state._activePortrait->SetPosition(scalePos(42.0f, 24.0f));
        state._activePortrait->SetScale(uniformSize(106.0f, 106.0f));
        state._activePortrait->SetDepth(0.16f);
        state._activePortrait->Draw();
    }

    const vector<GameplayState::HudCommandButton> commandButtons = buildHudCommandButtons(state);
    vector<shared_ptr<Sprite>> commandIcons;
    if (state._selectionKind == SandforgeSelectionKind::HQ)
    {
        commandIcons = { state._queuePreviewIcons[SandforgeUnitType::Worker], state._uiDecorSprites["barracks"], state._uiDecorSprites["factory"] };
    }
    else if (state._selectionKind == SandforgeSelectionKind::Barracks)
    {
        commandIcons = { state._queuePreviewIcons[SandforgeUnitType::Soldier], state._queuePreviewIcons[SandforgeUnitType::Defender], state._queuePreviewIcons[SandforgeUnitType::SiegeUnit] };
    }
    else if (state._selectionKind == SandforgeSelectionKind::Factory)
    {
        commandIcons = { state._queuePreviewIcons[SandforgeUnitType::RangerMech], state._queuePreviewIcons[SandforgeUnitType::SiegeUnit] };
    }
    else if (state._selectionKind == SandforgeSelectionKind::Node)
    {
        commandIcons = { state._uiDecorSprites["nodehub"], state._uiDecorSprites["tower"] };
    }

    for (size_t index = 0; index < state._commandSlotBackgrounds.size(); ++index)
    {
        if (state._commandSlotBackgrounds[index] == nullptr) continue;
        const bool enabled = index < commandButtons.size() ? commandButtons[index].enabled : false;
        const bool hovered = index < commandButtons.size() && commandButtons[index].hotkey == state._hoveredCommandHotkey;
        state._commandSlotBackgrounds[index]->SetDepth(hovered ? 0.162f : (enabled ? 0.158f : 0.152f));
        state._commandSlotBackgrounds[index]->Draw();
    }

    for (size_t index = 0; index < commandIcons.size() && index < state._commandSlotBackgrounds.size() && index < commandButtons.size(); ++index)
    {
        if (commandIcons[index] == nullptr) continue;
        commandIcons[index]->SetPosition(commandButtons[index].position + uniformSize(9.0f, 9.0f));
        commandIcons[index]->SetScale(uniformSize(40.0f, 40.0f));
        commandIcons[index]->SetDepth(commandButtons[index].enabled ? 0.165f : 0.153f);
        commandIcons[index]->Draw();
    }

    size_t minimapIndex = 0;
    const vec2 minimapDisplayOrigin = scalePos(528.0f, 28.0f);
    const vec2 minimapSize = scaleSize(212.0f, 96.0f);
    auto drawBlip = [&](const SandforgeVec2& worldPosition, const vec2& size)
    {
        if (minimapIndex >= state._minimapBlips.size() || state._minimapBlips[minimapIndex] == nullptr)
        {
            return;
        }

        const float x = minimapDisplayOrigin.x + ((worldPosition.x / 2560.0f) * minimapSize.x);
        const float y = minimapDisplayOrigin.y + ((worldPosition.y / 720.0f) * minimapSize.y);
        state._minimapBlips[minimapIndex]->SetPosition(vec2(x, y));
        state._minimapBlips[minimapIndex]->SetScale(size);
        state._minimapBlips[minimapIndex]->SetDepth(0.165f);
        state._minimapBlips[minimapIndex]->Draw();
        ++minimapIndex;
    };

    const SandforgeBuilding* friendlyHQ = state._world.findPrimaryBuilding(1, SandforgeBuildingType::HQ);
    const SandforgeBuilding* enemyHQ = state._world.findPrimaryBuilding(2, SandforgeBuildingType::HQ);
    if (friendlyHQ != nullptr) drawBlip(friendlyHQ->position, uniformSize(20.0f, 20.0f));
    if (enemyHQ != nullptr) drawBlip(enemyHQ->position, uniformSize(22.0f, 22.0f));
    for (const SandforgeResourceNode& node : nodes)
    {
        drawBlip(node.position, uniformSize(16.0f, 16.0f));
    }

    float healthRatio = 1.0f;
    if (state._selectionKind == SandforgeSelectionKind::Unit)
    {
        const SandforgeUnit* unit = state.getSelectedUnit();
        if (unit != nullptr && unit->maxHp > 0.0f)
        {
            healthRatio = unit->hp / unit->maxHp;
        }
    }
    else if (state._selectionKind == SandforgeSelectionKind::HQ)
    {
        const SandforgeBuilding* building = state._world.findPrimaryBuilding(1, SandforgeBuildingType::HQ);
        if (building != nullptr && building->maxHp > 0.0f) healthRatio = building->hp / building->maxHp;
    }
    else if (state._selectionKind == SandforgeSelectionKind::Barracks)
    {
        const SandforgeBuilding* building = state.getSelectedBuilding();
        if (building != nullptr && building->maxHp > 0.0f) healthRatio = building->hp / building->maxHp;
    }
    else if (state._selectionKind == SandforgeSelectionKind::Factory)
    {
        const SandforgeBuilding* building = state.getSelectedBuilding();
        if (building != nullptr && building->maxHp > 0.0f) healthRatio = building->hp / building->maxHp;
    }

    if (state._healthBarFrame != nullptr)
    {
        state._healthBarFrame->SetTintColor(0.18f, 0.05f, 0.05f, 1.0f);
        state._healthBarFrame->SetPosition(scalePos(154.0f, 38.0f));
        state._healthBarFrame->SetScale(scaleSize(96.0f, 10.0f));
        state._healthBarFrame->SetDepth(0.16f);
        state._healthBarFrame->Draw();
    }
    if (state._healthBarFill != nullptr)
    {
        state._healthBarFill->SetTintColor(0.88f, 0.14f, 0.14f, 1.0f);
        state._healthBarFill->SetPosition(scalePos(154.0f, 38.0f));
        state._healthBarFill->SetScale(scaleSize(96.0f * glm::clamp(healthRatio, 0.0f, 1.0f), 10.0f));
        state._healthBarFill->SetDepth(0.165f);
        state._healthBarFill->Draw();
    }

    const SandforgeBuilding* selectedBuilding = nullptr;
    if (state._selectionKind == SandforgeSelectionKind::HQ)
    {
        selectedBuilding = state._world.findPrimaryBuilding(1, SandforgeBuildingType::HQ);
    }
    else if (state._selectionKind == SandforgeSelectionKind::Barracks || state._selectionKind == SandforgeSelectionKind::Factory)
    {
        selectedBuilding = state.getSelectedBuilding();
    }

    if (selectedBuilding != nullptr)
    {
        const float startX = 304.0f * sx;
        const float startY = 32.0f * sy;
        const float slotSpacing = 30.0f * sx;
        const float frameWidth = 22.0f * sx;
        const float frameHeight = 6.0f * sy;

        for (size_t index = 0; index < selectedBuilding->productionQueue.size(); ++index)
        {
            const auto iconIterator = state._queuePreviewIcons.find(selectedBuilding->productionQueue[index].unitType);
            if (iconIterator == state._queuePreviewIcons.end() || iconIterator->second == nullptr)
            {
                continue;
            }

            iconIterator->second->SetPosition(vec2(startX + (static_cast<float>(index) * slotSpacing), startY));
            iconIterator->second->SetScale(uniformSize(22.0f, 22.0f));
            iconIterator->second->SetDepth(0.17f);
            iconIterator->second->Draw();

            if (state._queueProgressFrame != nullptr)
            {
                state._queueProgressFrame->SetPosition(startX + (static_cast<float>(index) * slotSpacing), startY - 6.0f);
                state._queueProgressFrame->SetScale(vec2(frameWidth, frameHeight));
                state._queueProgressFrame->SetDepth(0.165f);
                state._queueProgressFrame->Draw();
            }

            if (state._queueProgressBar != nullptr)
            {
                float progress = 0.15f;
                if (index == 0 && selectedBuilding->productionQueue[index].totalTime > 0.0f)
                {
                    progress = 1.0f - (selectedBuilding->productionQueue[index].remainingTime / selectedBuilding->productionQueue[index].totalTime);
                }

                progress = glm::clamp(progress, 0.0f, 1.0f);
                state._queueProgressBar->SetPosition(startX + (static_cast<float>(index) * slotSpacing), startY - 6.0f);
                state._queueProgressBar->SetScale(vec2(frameWidth * progress, frameHeight));
                state._queueProgressBar->SetDepth(0.17f);
                state._queueProgressBar->Draw();
            }
        }
    }

    for (size_t index = 0; index < nodes.size() && index < 2; ++index)
    {
        shared_ptr<Sprite> nodeIcon = nodes[index].resourceType == SandforgeResourceType::Metal ? state._uiDecorSprites["metal_node"] : state._uiDecorSprites["energy_node"];
        if (nodeIcon == nullptr) continue;
        nodeIcon->SetPosition(scalePos(308.0f + (static_cast<float>(index) * 58.0f), 90.0f));
        nodeIcon->SetScale(uniformSize(26.0f, 26.0f));
        nodeIcon->SetDepth(0.162f);
        nodeIcon->Draw();

        shared_ptr<Sprite> ownerRing = nodes[index].ownerId == 2 ? state._enemySelectionRing : state._friendlySelectionRing;
        if (ownerRing != nullptr && nodes[index].ownerId != 0)
        {
            ownerRing->SetPosition(scalePos(304.0f + (static_cast<float>(index) * 58.0f), 86.0f));
            ownerRing->SetScale(uniformSize(34.0f, 34.0f));
            ownerRing->SetDepth(0.158f);
            ownerRing->Draw();
        }
    }

    if (state._tooltipPanel != nullptr && state.hasCommandTooltip())
    {
        state._tooltipPanel->SetPosition(state.getCommandTooltipPosition());
        state._tooltipPanel->SetScale(scaleSize(270.0f, 112.0f));
        state._tooltipPanel->SetDepth(0.175f);
        state._tooltipPanel->Draw();
    }
}

vector<GameplayState::HudCommandButton> GameplayHudRenderer::buildHudCommandButtons(const GameplayState& state) const
{
    vector<GameplayState::HudCommandButton> buttons;
    const float sx = state._uiViewportSize.x / 1280.0f;
    const float sy = state._uiViewportSize.y / 720.0f;
    const vec2 origin(940.0f * sx, 28.0f * sy);
    const vec2 size(64.0f * sx, 50.0f * sy);

    auto pushButton = [&](int hotkey, const string& label, bool enabled = true)
    {
        const int index = static_cast<int>(buttons.size());
        const int column = index % 2;
        const int row = index / 2;
        buttons.push_back({ origin + vec2(static_cast<float>(column) * (78.0f * sx), static_cast<float>(row) * (38.0f * sy)), size, hotkey, label, enabled });
    };

    if (state._selectionKind == SandforgeSelectionKind::HQ)
    {
        pushButton(GLFW_KEY_1, "Worker");
        pushButton(GLFW_KEY_B, "Barracks");
        pushButton(GLFW_KEY_F, "Factory");
    }
    else if (state._selectionKind == SandforgeSelectionKind::Barracks)
    {
        pushButton(GLFW_KEY_2, "Soldier");
        pushButton(GLFW_KEY_3, "Defender");
    }
    else if (state._selectionKind == SandforgeSelectionKind::Factory)
    {
        pushButton(GLFW_KEY_6, "Mech");
        pushButton(GLFW_KEY_7, "Siege");
    }
    else if (state._selectionKind == SandforgeSelectionKind::Node)
    {
        const bool owned = state._selectedNodeIndex >= 0 &&
            state._selectedNodeIndex < static_cast<int>(state._world.getNodes().size()) &&
            state._world.getNodes()[state._selectedNodeIndex].ownerId == 1;
        pushButton(GLFW_KEY_4, "NodeHub", owned);
        pushButton(GLFW_KEY_8, "Tower", owned);
    }

    return buttons;
}

void GameplayHudRenderer::updateHoveredHudCommand(GameplayState& state, const vec2& cursorScreenPosition) const
{
    state._hoveredCommandHotkey = 0;
    const vector<GameplayState::HudCommandButton> buttons = buildHudCommandButtons(state);
    for (const GameplayState::HudCommandButton& button : buttons)
    {
        if (!button.enabled)
        {
            continue;
        }

        if (cursorScreenPosition.x >= button.position.x && cursorScreenPosition.x <= button.position.x + button.size.x &&
            cursorScreenPosition.y >= button.position.y && cursorScreenPosition.y <= button.position.y + button.size.y)
        {
            state._hoveredCommandHotkey = button.hotkey;
            return;
        }
    }
}

bool GameplayHudRenderer::handleHudClick(GameplayState& state, const vec2& cursorScreenPosition, const vec2& cursorWorldPosition) const
{
    const vector<GameplayState::HudCommandButton> buttons = buildHudCommandButtons(state);
    for (const GameplayState::HudCommandButton& button : buttons)
    {
        if (!button.enabled)
        {
            continue;
        }

        if (cursorScreenPosition.x >= button.position.x && cursorScreenPosition.x <= button.position.x + button.size.x &&
            cursorScreenPosition.y >= button.position.y && cursorScreenPosition.y <= button.position.y + button.size.y)
        {
            return activateHudCommand(state, button.hotkey, cursorWorldPosition);
        }
    }

    return false;
}

bool GameplayHudRenderer::activateHudCommand(GameplayState& state, int hotkey, const vec2& cursorWorldPosition) const
{
    if (hotkey == GLFW_KEY_1)
    {
        if (state._selectionKind == SandforgeSelectionKind::HQ)
        {
            return state._world.queueProduction(1, SandforgeBuildingType::HQ, SandforgeUnitType::Worker);
        }
        state._statusText = "Select Headquarters to train Workers.";
        return false;
    }
    if (hotkey == GLFW_KEY_2)
    {
        if (state._selectionKind == SandforgeSelectionKind::Barracks && state._selectedBuildingId != 0)
        {
            return state._world.queueProduction(1, state._selectedBuildingId, SandforgeUnitType::Soldier);
        }
        state._statusText = "Select a Barracks to train Soldiers.";
        return false;
    }
    if (hotkey == GLFW_KEY_3)
    {
        if (state._selectionKind == SandforgeSelectionKind::Barracks && state._selectedBuildingId != 0)
        {
            return state._world.queueProduction(1, state._selectedBuildingId, SandforgeUnitType::Defender);
        }
        state._statusText = "Select a Barracks to train Defenders.";
        return false;
    }
    if (hotkey == GLFW_KEY_4 && state._selectionKind == SandforgeSelectionKind::Node)
    {
        state.beginBuildPreview(SandforgeBuildPreviewKind::NodeHub, cursorWorldPosition);
        return true;
    }
    if (hotkey == GLFW_KEY_B && state._selectionKind == SandforgeSelectionKind::HQ)
    {
        state.beginBuildPreview(SandforgeBuildPreviewKind::Barracks, cursorWorldPosition);
        return true;
    }
    if (hotkey == GLFW_KEY_F && state._selectionKind == SandforgeSelectionKind::HQ)
    {
        state.beginBuildPreview(SandforgeBuildPreviewKind::Factory, cursorWorldPosition);
        return true;
    }
    if (hotkey == GLFW_KEY_6)
    {
        if (state._selectionKind == SandforgeSelectionKind::Factory && state._selectedBuildingId != 0)
        {
            return state._world.queueProduction(1, state._selectedBuildingId, SandforgeUnitType::RangerMech);
        }
        state._statusText = "Select a Factory to train Mechs.";
        return false;
    }
    if (hotkey == GLFW_KEY_7)
    {
        if (state._selectionKind == SandforgeSelectionKind::Factory && state._selectedBuildingId != 0)
        {
            return state._world.queueProduction(1, state._selectedBuildingId, SandforgeUnitType::SiegeUnit);
        }
        state._statusText = "Select a Factory to train Siege Units.";
        return false;
    }
    if (hotkey == GLFW_KEY_8 && state._selectionKind == SandforgeSelectionKind::Node)
    {
        state.beginBuildPreview(SandforgeBuildPreviewKind::DefenseTower, cursorWorldPosition);
        return true;
    }

    return false;
}
