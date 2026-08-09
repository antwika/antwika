#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

#include <antwika/enums/Enumeration.hpp>

namespace antwika::game
{

    enum class BuildingKind : std::uint8_t
    {
        House = 0,
        Farm,
        ClayPit,
        Workshop,
        Storage,
        Market,
        Well,
        Doctor,
        FireStation,
        EngineerPost,
    };

    [[nodiscard]] constexpr BuildingKind enumBound(BuildingKind) noexcept
    {
        return BuildingKind::EngineerPost;
    }

    inline constexpr std::size_t kBuildingKindCount =
        antwika::enums::kCount<BuildingKind>;

    [[nodiscard]] constexpr std::size_t buildingKindIndex(
        const BuildingKind kind) noexcept
    {
        return antwika::enums::index(kind);
    }

    [[nodiscard]] constexpr bool consumes(BuildingKind kind) noexcept
    {
        constexpr std::array<bool, kBuildingKindCount> consuming{
            true,
            false,
            false,
            false,
            false,
            false,
            false,
            false,
            false,
            false,
        };

        return antwika::enums::pick(consuming, kind);
    }

    [[nodiscard]] constexpr bool sendsWalkers(BuildingKind kind) noexcept
    {
        constexpr std::array<bool, kBuildingKindCount> sending{
            false,
            true,
            true,
            true,
            false,
            true,
            true,
            true,
            true,
            true,
        };

        return antwika::enums::pick(sending, kind);
    }

    [[nodiscard]] constexpr std::string_view buildingKindName(
        BuildingKind kind) noexcept
    {
        constexpr std::array<std::string_view, kBuildingKindCount> names{
            "house",
            "farm",
            "clay_pit",
            "workshop",
            "storage",
            "market",
            "well",
            "doctor",
            "fire_station",
            "engineer_post"};

        return antwika::enums::pick(names, kind);
    }

    [[nodiscard]] constexpr std::optional<BuildingKind> buildingKindFromName(
        std::string_view name) noexcept
    {
        for (std::size_t index = 0; index < kBuildingKindCount; ++index)
        {
            const auto kind = static_cast<BuildingKind>(index);

            if (buildingKindName(kind) == name)
            {
                return kind;
            }
        }

        return std::nullopt;
    }

    static_assert(buildingKindName(BuildingKind::House) == "house");
    static_assert(
        buildingKindFromName("engineer_post") == BuildingKind::EngineerPost);
    static_assert(!buildingKindFromName("architect_post").has_value());
    static_assert(!buildingKindFromName("tower").has_value());

    static_assert(!consumes(BuildingKind::Storage));
    static_assert(!sendsWalkers(BuildingKind::Storage));

}
