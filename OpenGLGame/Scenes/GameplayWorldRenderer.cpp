#include "pch.h"
#include "GameplayWorldRenderer.h"

#include "GameplayState.h"
#include "Sprite.h"

void GameplayWorldRenderer::renderEffects(GameplayState& state) const
{
    if (state._friendlySelectionRing == nullptr)
    {
        return;
    }

    if (state._selectionKind == SandforgeSelectionKind::Unit)
    {
        const SandforgeUnit* unit = state.getSelectedUnit();
        if (unit != nullptr)
        {
            const vec2 size = SandforgeDatabase::getUnit(unit->unitType).visuals.spriteSize;
            state._friendlySelectionRing->SetPosition(unit->position.x - (size.x * 0.7f), unit->position.y - (size.y * 0.7f));
            state._friendlySelectionRing->SetScale(size * 1.4f);
            state._friendlySelectionRing->SetDepth(-0.03f);
            state._friendlySelectionRing->Draw();

            if (state._rallyGuideDot != nullptr)
            {
                state._rallyGuideDot->SetPosition(unit->moveTarget.x - 9.0f, unit->moveTarget.y - 9.0f);
                state._rallyGuideDot->SetDepth(0.09f);
                state._rallyGuideDot->Draw();
            }
        }
    }

    if (state._selectionKind == SandforgeSelectionKind::Node &&
        state._selectedNodeIndex >= 0 &&
        state._selectedNodeIndex < static_cast<int>(state._world.getNodes().size()))
    {
        const SandforgeResourceNode& node = state._world.getNodes()[state._selectedNodeIndex];
        state._friendlySelectionRing->SetPosition(node.position.x - 46.0f, node.position.y - 46.0f);
        state._friendlySelectionRing->SetScale(vec2(92.0f, 92.0f));
        state._friendlySelectionRing->SetDepth(-0.03f);
        state._friendlySelectionRing->Draw();
    }

    if (state._selectionKind == SandforgeSelectionKind::HQ)
    {
        const SandforgeBuilding* hq = state._world.findPrimaryBuilding(state.getLocalPlayerId(), SandforgeBuildingType::HQ);
        if (hq != nullptr)
        {
            state._friendlySelectionRing->SetPosition(hq->position.x - 58.0f, hq->position.y - 58.0f);
            state._friendlySelectionRing->SetScale(vec2(116.0f, 116.0f));
            state._friendlySelectionRing->SetDepth(-0.03f);
            state._friendlySelectionRing->Draw();
        }
    }

    if (state._selectionKind == SandforgeSelectionKind::Barracks)
    {
        const SandforgeBuilding* barracks = state.getSelectedBuilding();
        if (barracks != nullptr)
        {
            state._friendlySelectionRing->SetPosition(barracks->position.x - 50.0f, barracks->position.y - 50.0f);
            state._friendlySelectionRing->SetScale(vec2(100.0f, 100.0f));
            state._friendlySelectionRing->SetDepth(-0.03f);
            state._friendlySelectionRing->Draw();
        }
    }

    if (state._selectionKind == SandforgeSelectionKind::Factory)
    {
        const SandforgeBuilding* factory = state.getSelectedBuilding();
        if (factory != nullptr)
        {
            state._friendlySelectionRing->SetPosition(factory->position.x - 54.0f, factory->position.y - 54.0f);
            state._friendlySelectionRing->SetScale(vec2(108.0f, 108.0f));
            state._friendlySelectionRing->SetDepth(-0.03f);
            state._friendlySelectionRing->Draw();
        }
    }

    if (state._selectionKind == SandforgeSelectionKind::NodeHub)
    {
        for (const SandforgeBuilding& building : state._world.getBuildings())
        {
            if (!building.alive || building.ownerId != state.getLocalPlayerId() || building.buildingType != SandforgeBuildingType::NodeHub)
            {
                continue;
            }

            state._friendlySelectionRing->SetPosition(building.position.x - 46.0f, building.position.y - 46.0f);
            state._friendlySelectionRing->SetScale(vec2(92.0f, 92.0f));
            state._friendlySelectionRing->SetDepth(-0.03f);
            state._friendlySelectionRing->Draw();
            break;
        }
    }

    if (state._selectionKind == SandforgeSelectionKind::DefenseTower)
    {
        for (const SandforgeBuilding& building : state._world.getBuildings())
        {
            if (!building.alive || building.ownerId != state.getLocalPlayerId() || building.buildingType != SandforgeBuildingType::DefenseTower)
            {
                continue;
            }

            state._friendlySelectionRing->SetPosition(building.position.x - 44.0f, building.position.y - 44.0f);
            state._friendlySelectionRing->SetScale(vec2(88.0f, 88.0f));
            state._friendlySelectionRing->SetDepth(-0.03f);
            state._friendlySelectionRing->Draw();
            break;
        }
    }

    const SandforgeBuilding* enemyHQ = state._world.findPrimaryBuilding(state.getEnemyPlayerId(), SandforgeBuildingType::HQ);
    if (enemyHQ != nullptr && state._enemySelectionRing != nullptr)
    {
        state._enemySelectionRing->SetPosition(enemyHQ->position.x - 54.0f, enemyHQ->position.y - 54.0f);
        state._enemySelectionRing->SetDepth(-0.02f);
        state._enemySelectionRing->Draw();
    }

    for (const SandforgeCombatEffect& effect : state._world.getCombatEffects())
    {
        if (state._attackEffectSprite == nullptr)
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
            state._attackEffectSprite->SetPosition(point.x - 7.0f, point.y - 7.0f);
            state._attackEffectSprite->SetDepth(0.10f);
            state._attackEffectSprite->Draw();
        }
    }

    for (const SandforgeResourceNode& node : state._world.getNodes())
    {
        if (node.harvestingWorkerId == 0 || state._rallyGuideDot == nullptr)
        {
            continue;
        }

        const float pulse = static_cast<float>(fmod(state._world.getElapsedTime() * 2.0 + static_cast<double>(node.id), 1.0));
        for (int index = 0; index < 3; ++index)
        {
            const float angle = pulse + (static_cast<float>(index) * 0.33f);
            const float offsetX = cos(angle * glm::two_pi<float>()) * 18.0f;
            const float offsetY = sin(angle * glm::two_pi<float>()) * 10.0f;
            state._rallyGuideDot->SetPosition(node.position.x + offsetX - 6.0f, node.position.y + offsetY + 20.0f);
            state._rallyGuideDot->SetScale(vec2(12.0f, 12.0f));
            state._rallyGuideDot->SetDepth(0.095f);
            state._rallyGuideDot->Draw();
        }
    }

    if (state._buildPreviewKind != SandforgeBuildPreviewKind::None && state._buildPreviewSprite != nullptr)
    {
        const SandforgeBuildingType previewType = state.getPreviewBuildingType();
        const vec2 previewSize = SandforgeDatabase::getBuilding(previewType).visuals.spriteSize;
        state._buildPreviewSprite->SetScale(previewSize);
        state._buildPreviewSprite->SetDepth(0.02f);
        state._buildPreviewSprite->Draw();

        const vec2 previewCenter = state._buildPreviewSprite->GetPosition() + (previewSize * 0.5f);
        const bool valid = state.isBuildPreviewValid(previewCenter);
        shared_ptr<Sprite> ring = valid ? state._friendlySelectionRing : state._enemySelectionRing;
        if (ring != nullptr)
        {
            ring->SetPosition(previewCenter.x - (previewSize.x * 0.65f), previewCenter.y - (previewSize.y * 0.65f));
            ring->SetScale(previewSize * 1.3f);
            ring->SetDepth(-0.01f);
            ring->Draw();
        }
    }
}
