#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include <antwika/enums/Enumeration.hpp>
#include <antwika/map/Marker.hpp>
#include <antwika/voxel/VoxelPosition.hpp>

namespace antwika::editor
{

    enum class EntityKind : std::uint8_t
    {
        Start,
        Exit,
        Lamp,
        Checkpoint,
        Food,
        Water,
        Character,
    };

    [[nodiscard]] constexpr EntityKind getLastEnumerator(EntityKind) noexcept
    {
        return EntityKind::Character;
    }

    inline constexpr std::array<std::string_view, enums::kCount<EntityKind>>
        kEntityNames{
            "Start", "Exit", "Lamp",
            "Checkpoint", "Food", "Water",
            "Character"};

    [[nodiscard]] constexpr std::optional<map::Marker> getMarkerOfEntity(
        const EntityKind kind) noexcept
    {
        switch (kind)
        {
        case EntityKind::Checkpoint:
            return map::Marker::Checkpoint;
        case EntityKind::Food:
            return map::Marker::Food;
        case EntityKind::Water:
            return map::Marker::Water;
        default:
            break;
        }

        return std::nullopt;
    }

    struct EntityPick final
    {
        std::optional<EntityKind> kind;

        voxel::VoxelPosition position{};

        std::size_t characterIndex = 0;

        std::optional<std::size_t> editingAxis;

        std::string pendingAxisText;

        bool dragging = false;

        bool dragUndoKept = false;
    };

}
