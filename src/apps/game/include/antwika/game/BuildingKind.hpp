#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace antwika::game
{

    /**
     * @brief What a building is for.
     *
     * A house is the only kind that consumes; the other four each send
     * one kind of walker out, which is what gives a walker somewhere to
     * come back from.
     *
     * Values are contiguous from zero, so a kind can index a table, and
     * the order is the order the atlas draws them in.
     */
    enum class BuildingKind : std::uint8_t
    {
        House = 0,      ///< Consumes what walkers bring it.
        FoodSource,     ///< Sends a food walker out.
        WaterSource,    ///< Sends a water walker out.
        FireStation,    ///< Sends a fireman out.
        ArchitectPost,  ///< Sends an architect out.
    };

    /**
     * @brief How many building kinds there are.
     */
    inline constexpr std::size_t kBuildingKindCount =
        static_cast<std::size_t>(BuildingKind::ArchitectPost) + 1;

    /**
     * @brief Get a kind's index, for addressing a per-kind table.
     * @param kind The kind to index.
     * @return The index, always below kBuildingKindCount.
     */
    [[nodiscard]] constexpr std::size_t buildingKindIndex(
        BuildingKind kind) noexcept
    {
        return static_cast<std::size_t>(kind);
    }

    /**
     * @brief Check whether a building consumes what is delivered to it.
     *
     * Asked about the house rather than listing the four sources, so a
     * sixth kind is a source without this having to be edited.
     *
     * @param kind The kind to ask about.
     * @return True for a house and nothing else.
     */
    [[nodiscard]] constexpr bool consumes(BuildingKind kind) noexcept
    {
        return kind == BuildingKind::House;
    }

    /**
     * @brief Check whether a building sends walkers out.
     * @param kind The kind to ask about.
     * @return True for every kind but the house.
     */
    [[nodiscard]] constexpr bool sendsWalkers(BuildingKind kind) noexcept
    {
        return !consumes(kind);
    }

    /**
     * @brief Get a kind's name.
     *
     * One table rather than one per caller, because a save file writes
     * the name rather than the index -- an index would renumber every
     * saved building the day a kind is inserted rather than appended,
     * and the file would load without complaining.
     *
     * @param kind The kind to name.
     * @return Its name, in the enumeration's own order.
     */
    [[nodiscard]] constexpr std::string_view buildingKindName(
        BuildingKind kind) noexcept
    {
        constexpr std::array<std::string_view, kBuildingKindCount> names{
            "house",
            "food_source",
            "water_source",
            "fire_station",
            "architect_post"};

        return names[buildingKindIndex(kind) % kBuildingKindCount];
    }

    /**
     * @brief Get the kind a name refers to.
     * @param name The name to look up.
     * @return The kind, or nullopt when no kind has that name.
     */
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
        buildingKindFromName("architect_post") == BuildingKind::ArchitectPost);
    static_assert(!buildingKindFromName("tower").has_value());

} // namespace antwika::game
