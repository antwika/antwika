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
     * A house is the only kind that consumes; most of the rest send
     * walkers out, which is what gives a walker somewhere to come back
     * from, and a storehouse sends nobody at all.
     *
     * Values are contiguous from zero, so a kind can index a table, and
     * the order is the order the atlas draws them in.
     *
     * **The order is a schema, in one direction only.** A save file
     * writes buildingKindName() rather than the index, so appending a
     * kind is free and reordering is not silently wrong -- but
     * TileAtlas.hpp addresses the art by index, so moving one here moves
     * every building tile after it.
     */
    enum class BuildingKind : std::uint8_t
    {
        House = 0,     ///< Consumes what walkers bring it.
        Farm,          ///< Grows food.
        ClayPit,       ///< Digs clay.
        Workshop,      ///< Turns clay into pottery.
        Storage,       ///< Holds any good, and sends nobody out.
        Market,        ///< Buys from a store and sells to houses.
        Well,          ///< Waters what its carrier walks past.
        Doctor,        ///< Keeps a district healthy.
        FireStation,   ///< Keeps a district safe.
        EngineerPost,  ///< Keeps a district standing.
    };

    /**
     * @brief How many building kinds there are.
     */
    inline constexpr std::size_t kBuildingKindCount =
        static_cast<std::size_t>(BuildingKind::EngineerPost) + 1;

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
     * **A table rather than a comparison against one enumerator.**
     * It used to read `kind == House`, which was exact while a house was
     * the only kind that ate anything; it stops being exact the moment a
     * workshop consumes clay to make pottery, and a predicate written as
     * one name is a predicate nobody can extend without rewriting every
     * caller's assumption at the same time.
     *
     * @param kind The kind to ask about.
     * @return True for a kind whose stock drains and whose running out
     * loses it.
     */
    [[nodiscard]] constexpr bool consumes(BuildingKind kind) noexcept
    {
        constexpr std::array<bool, kBuildingKindCount> consuming{
            true,   // House
            false,  // Farm
            false,  // ClayPit
            false,  // Workshop
            false,  // Storage
            false,  // Market
            false,  // Well
            false,  // Doctor
            false,  // FireStation
            false,  // EngineerPost
        };

        return consuming[buildingKindIndex(kind) % kBuildingKindCount];
    }

    /**
     * @brief Check whether a building sends walkers out.
     *
     * **A table rather than the negation of consumes().** The two were
     * exact opposites while every kind either ate or walked; a
     * storehouse does neither -- goods are carted to it and carted away
     * again, and it never sends anybody -- so the negation would have it
     * emitting a walker with nothing to do.
     *
     * @param kind The kind to ask about.
     * @return True for a kind that has somebody to send.
     */
    [[nodiscard]] constexpr bool sendsWalkers(BuildingKind kind) noexcept
    {
        constexpr std::array<bool, kBuildingKindCount> sending{
            false,  // House
            true,   // Farm
            true,   // ClayPit
            true,   // Workshop
            false,  // Storage
            true,   // Market
            true,   // Well
            true,   // Doctor
            true,   // FireStation
            true,   // EngineerPost
        };

        return sending[buildingKindIndex(kind) % kBuildingKindCount];
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
            "farm",
            "clay_pit",
            "workshop",
            "storage",
            "market",
            "well",
            "doctor",
            "fire_station",
            "engineer_post"};

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
        buildingKindFromName("engineer_post") == BuildingKind::EngineerPost);
    static_assert(!buildingKindFromName("architect_post").has_value());
    static_assert(!buildingKindFromName("tower").has_value());

    // A storehouse neither eats nor walks.
    // Which is what stopped the two being each other's negation.
    static_assert(!consumes(BuildingKind::Storage));
    static_assert(!sendsWalkers(BuildingKind::Storage));

} // namespace antwika::game
