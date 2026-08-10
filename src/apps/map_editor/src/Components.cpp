#include "antwika/map_editor/Components.hpp"

#include <variant>

namespace antwika::map_editor
{

    MarkerKind markerKindOf(const tilemap::Entity &entity) noexcept
    {
        if (std::holds_alternative<tilemap::Transition>(entity))
        {
            return MarkerKind::Transition;
        }

        if (std::holds_alternative<tilemap::BoatEmbark>(entity))
        {
            return MarkerKind::Boat;
        }

        if (std::holds_alternative<tilemap::SpawnPoint>(entity))
        {
            return MarkerKind::Spawn;
        }

        if (std::holds_alternative<tilemap::Pickup>(entity))
        {
            return MarkerKind::Pickup;
        }

        if (std::holds_alternative<tilemap::Npc>(entity))
        {
            return MarkerKind::Npc;
        }

        return MarkerKind::Trigger;
    }

    geometry::GridCell entityCellOf(
        const tilemap::Entity &entity) noexcept
    {
        return std::visit(
            [](const auto &kind) { return kind.at; }, entity);
    }

}
