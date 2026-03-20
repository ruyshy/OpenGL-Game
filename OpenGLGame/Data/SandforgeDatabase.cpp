#include "pch.h"
#include "SandforgeDatabase.h"

namespace
{
    const array<SandforgeUnitDefinition, 6> kUnits =
    {{
        { SandforgeUnitType::Worker, "Worker", 50, 0, 60.0f, 5.0f, 5.0f, 1.2f, 24.0f, 110.0f, 8.0f, true, true, false, false, SandforgeDamageType::Normal, SandforgeArmorType::Light, { "Assets/Units/worker.png", "Assets/Units/worker_idle.csv", vec2(42.0f, 42.0f) } },
        { SandforgeUnitType::Soldier, "Soldier", 80, 0, 110.0f, 14.0f, 14.0f, 1.0f, 48.0f, 100.0f, 10.0f, false, false, true, false, SandforgeDamageType::Normal, SandforgeArmorType::Light, { "Assets/Units/soldier.png", "Assets/Units/soldier_walk.csv", vec2(44.0f, 44.0f) } },
        { SandforgeUnitType::Defender, "Defender", 120, 10, 220.0f, 10.0f, 10.0f, 1.2f, 40.0f, 72.0f, 14.0f, false, false, true, false, SandforgeDamageType::Normal, SandforgeArmorType::Armored, { "Assets/Units/defender.png", "Assets/Units/defender_walk.csv", vec2(52.0f, 52.0f) } },
        { SandforgeUnitType::Raider, "Raider", 90, 20, 85.0f, 18.0f, 18.0f, 0.9f, 36.0f, 138.0f, 11.0f, false, false, true, false, SandforgeDamageType::Piercing, SandforgeArmorType::Light, { "Assets/Units/raider.png", "Assets/Units/raider_walk.csv", vec2(44.0f, 44.0f) } },
        { SandforgeUnitType::RangerMech, "Ranger Mech", 100, 35, 95.0f, 22.0f, 22.0f, 1.4f, 170.0f, 82.0f, 15.0f, false, false, true, false, SandforgeDamageType::Piercing, SandforgeArmorType::Light, { "Assets/Units/ranger_mech.png", "Assets/Units/ranger_mech_walk.csv", vec2(50.0f, 50.0f) } },
        { SandforgeUnitType::SiegeUnit, "Siege Unit", 160, 60, 170.0f, 18.0f, 48.0f, 2.0f, 190.0f, 60.0f, 20.0f, false, false, true, true, SandforgeDamageType::Siege, SandforgeArmorType::Armored, { "Assets/Units/siege_unit.png", "Assets/Units/siege_unit_walk.csv", vec2(56.0f, 56.0f) } }
    }};

    const array<SandforgeBuildingDefinition, 5> kBuildings =
    {{
        { SandforgeBuildingType::HQ, "Headquarters", 0, 0, 3000.0f, 0.0f, true, true, false, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, { SandforgeUnitType::Worker }, { "Assets/Buildings/hq.png", "", vec2(112.0f, 112.0f) } },
        { SandforgeBuildingType::Barracks, "Barracks", 180, 0, 900.0f, 20.0f, false, false, false, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, { SandforgeUnitType::Soldier, SandforgeUnitType::Defender, SandforgeUnitType::Raider }, { "Assets/Buildings/barracks.png", "", vec2(96.0f, 96.0f) } },
        { SandforgeBuildingType::Factory, "Factory", 220, 80, 1100.0f, 28.0f, false, false, false, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, { SandforgeUnitType::RangerMech, SandforgeUnitType::SiegeUnit }, { "Assets/Buildings/factory.png", "", vec2(104.0f, 104.0f) } },
        { SandforgeBuildingType::NodeHub, "Node Hub", 140, 40, 700.0f, 18.0f, false, false, false, 1.25f, 180.0f, 0.0f, 0.0f, 0.0f, {}, { "Assets/Buildings/node_hub.png", "", vec2(82.0f, 82.0f) } },
        { SandforgeBuildingType::DefenseTower, "Defense Tower", 120, 20, 650.0f, 16.0f, false, false, true, 1.0f, 0.0f, 26.0f, 185.0f, 1.3f, {}, { "Assets/Buildings/defense_tower.png", "", vec2(72.0f, 72.0f) } }
    }};

    const array<SandforgeNodeDefinition, 2> kNodes =
    {{
        { "Metal Node", SandforgeResourceType::Metal, 5.0f, 10, 5.0f, { "Assets/Nodes/metal_node.png", "", vec2(72.0f, 72.0f) } },
        { "Energy Core", SandforgeResourceType::Energy, 5.0f, 6, 5.0f, { "Assets/Nodes/energy_core.png", "", vec2(72.0f, 72.0f) } }
    }};

    const SandforgeMatchRules kRules{};
}

const SandforgeUnitDefinition& SandforgeDatabase::getUnit(SandforgeUnitType type)
{
    return kUnits[static_cast<size_t>(type)];
}

const SandforgeBuildingDefinition& SandforgeDatabase::getBuilding(SandforgeBuildingType type)
{
    return kBuildings[static_cast<size_t>(type)];
}

const SandforgeNodeDefinition& SandforgeDatabase::getNode(SandforgeResourceType type)
{
    return kNodes[static_cast<size_t>(type)];
}

const SandforgeMatchRules& SandforgeDatabase::getMatchRules()
{
    return kRules;
}
