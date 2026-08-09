#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

#include <antwika/enums/Enumeration.hpp>

#include "antwika/game/BuildingKind.hpp"

namespace antwika::game
{

    enum class BuildTool : std::uint8_t
    {
        Road = 0,
        House,
        Farm,
        ClayPit,
        Workshop,
        Storage,
        Market,
        Well,
        Doctor,
        FireStation,
        EngineerPost,

        Raze,
    };

    [[nodiscard]] constexpr BuildTool enumBound(BuildTool) noexcept
    {
        return BuildTool::Raze;
    }

    inline constexpr std::size_t kBuildToolCount =
        antwika::enums::kCount<BuildTool>;

    [[nodiscard]] constexpr std::size_t buildToolIndex(
        const BuildTool tool) noexcept
    {
        return antwika::enums::index(tool);
    }

    [[nodiscard]] constexpr std::optional<BuildingKind> buildingKindOf(
        BuildTool tool) noexcept
    {
        constexpr std::array<
            std::optional<BuildingKind>, kBuildToolCount> places{
            std::nullopt,
            BuildingKind::House,
            BuildingKind::Farm,
            BuildingKind::ClayPit,
            BuildingKind::Workshop,
            BuildingKind::Storage,
            BuildingKind::Market,
            BuildingKind::Well,
            BuildingKind::Doctor,
            BuildingKind::FireStation,
            BuildingKind::EngineerPost,
            std::nullopt};

        return antwika::enums::pick(places, tool);
    }

    [[nodiscard]] constexpr bool placesBuilding(BuildTool tool) noexcept
    {
        return buildingKindOf(tool).has_value();
    }

    [[nodiscard]] constexpr bool dragsOut(const BuildTool tool) noexcept
    {
        return tool == BuildTool::Road || tool == BuildTool::House;
    }

    static_assert(
        []
        {
            for (std::size_t kind = 0; kind < kBuildingKindCount; ++kind)
            {
                bool found = false;

                for (std::size_t tool = 0; tool < kBuildToolCount; ++tool)
                {
                    found = found
                        || buildingKindOf(static_cast<BuildTool>(tool))
                               == static_cast<BuildingKind>(kind);
                }

                if (!found)
                {
                    return false;
                }
            }

            return true;
        }(),
        "every building kind needs a tool that places it");

    static_assert(
        []
        {
            for (std::size_t at = 0; at < kBuildToolCount; ++at)
            {
                const auto tool = static_cast<BuildTool>(at);

                if (dragsOut(tool) || tool == BuildTool::Raze)
                {
                    continue;
                }

                if (!buildingKindOf(tool).has_value())
                {
                    return false;
                }
            }

            return true;
        }(),
        "a tool that neither drags nor razes must place a building");

    static_assert(!buildingKindOf(BuildTool::Road).has_value());
    static_assert(!buildingKindOf(BuildTool::Raze).has_value());
    static_assert(buildingKindOf(BuildTool::House) == BuildingKind::House);
    static_assert(
        buildingKindOf(BuildTool::EngineerPost)
        == BuildingKind::EngineerPost);
}
