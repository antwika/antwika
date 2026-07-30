#pragma once

#include <cstdint>
#include <optional>

#include "antwika/game/Cell.hpp"
#include "antwika/game/Resource.hpp"

namespace antwika::game
{

    /**
     * @brief What a building is for.
     */
    enum class BuildingKind : std::uint8_t
    {
        House,          ///< Consumes what walkers bring it.
        FoodSource,     ///< Spawns a food walker.
        WaterSource,    ///< Spawns a water walker.
        FireStation,    ///< Spawns a fireman.
        ArchitectPost,  ///< Spawns an architect.
    };

    /**
     * @brief A building's stock of one resource.
     */
    struct Stock
    {
        Resource resource = Resource::Food;
        std::int32_t held = 0;
        std::int32_t capacity = 100;

        /**
         * @brief Compare two stocks.
         * @param other The stock to compare against.
         * @return True when the resource, the amount and the capacity all
         * match.
         */
        [[nodiscard]] bool operator==(const Stock &other) const = default;
    };

    /**
     * @brief How many ticks a second of game time is.
     *
     * An assumption stated once rather than a fact the code can read: the
     * pacing lives in the composition root, and a headless run is not
     * paced at all, so nothing here can ask what rate a run turned out to
     * have.
     * Every period below is derived from it, so changing the pace is one
     * edit here rather than one per constant.
     *
     * It is the reciprocal of main.cpp's TickPacer interval: 80 ms a tick
     * is twelve and a half of them a second.
     * Whole ticks are what a period can be counted in, so the half is
     * dropped and every period below comes out a few per cent short of the
     * wall-clock time its comment names -- which is the right way round,
     * since a spawn arriving slightly early is better than a building
     * starving while it waits.
     */
    inline constexpr std::int32_t kTicksPerSecond = 12;

    /**
     * @brief The most risk a building can carry before it is gone.
     */
    inline constexpr std::int32_t kMaxRisk = 100;

    /**
     * @brief How much risk a passing fireman or architect takes off.
     */
    inline constexpr std::int32_t kRiskRelief = 25;

    /**
     * @brief Ticks between one point of risk and the next.
     *
     * One a second, so an untended building reaches kMaxRisk after a
     * hundred seconds and is gone.
     */
    inline constexpr std::int32_t kRiskPeriodTicks = kTicksPerSecond;

    /**
     * @brief Ticks between one unit of stock draining and the next.
     *
     * One every thirty seconds, so a building filled to a capacity of a
     * hundred lasts fifty minutes and one just built -- ten per cent of
     * that -- lasts five.
     * That is longer than kSpawnPeriodTicks on purpose: a house has to
     * outlive the wait for the first walker, or nothing could ever be
     * delivered to it.
     */
    inline constexpr std::int32_t kDrainPeriodTicks = 30 * kTicksPerSecond;

    /**
     * @brief Ticks between one spawn and the next.
     *
     * One walker a minute, which is sixty seconds worth of the tick rate
     * assumed by kTicksPerSecond.
     */
    inline constexpr std::int32_t kSpawnPeriodTicks = 60 * kTicksPerSecond;

    /**
     * @brief What a building is on the grid.
     *
     * Where it is lives in a separate Cell component, as a walker's does,
     * so what a building is and where it stands are told apart by which
     * components an entity carries.
     *
     * The three counters are held per building rather than derived from
     * the tick, so that two buildings put up at different moments drain
     * and spawn on their own schedule instead of all at once on the
     * minute.
     * They count down rather than up, so nothing has to remember when the
     * last one happened.
     */
    struct Building
    {
        BuildingKind kind = BuildingKind::House;
        Stock stock{};

        /** @brief Risk of burning down, relieved by a fireman. */
        std::int32_t fireRisk = 0;

        /** @brief Risk of collapsing, relieved by an architect. */
        std::int32_t collapseRisk = 0;

        /** @brief Ticks until the next unit drains. */
        std::int32_t drainIn = kDrainPeriodTicks;

        /** @brief Ticks until the next point of risk. */
        std::int32_t riskIn = kRiskPeriodTicks;

        /** @brief Ticks until the next walker is spawned. */
        std::int32_t spawnIn = kSpawnPeriodTicks;

        /**
         * @brief Compare two buildings.
         * @param other The building to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(const Building &other) const = default;
    };

    /**
     * @brief What a click with the left button puts down.
     *
     * Simulation state rather than a renderer's idea of a mode, for the
     * reason the camera is: which tool is selected decides what a
     * recorded click means, so a replay has to arrive at the same one.
     * It is chosen with the number keys, which are input like any other
     * and are therefore recorded and replayed -- see Events.hpp for why
     * there is no event of our own for choosing one.
     */
    enum class BuildTool : std::uint8_t
    {
        Path,           ///< The default: lay a road.
        House,
        FoodSource,
        WaterSource,
        FireStation,
        ArchitectPost,
    };

    /**
     * @brief Get which building a tool puts down.
     * @param tool The selected tool.
     * @return The building it builds, or nullopt for the road tool, which
     * builds none.
     */
    [[nodiscard]] constexpr std::optional<BuildingKind> buildingFor(
        BuildTool tool) noexcept
    {
        if (tool == BuildTool::Path)
        {
            return std::nullopt;
        }

        // The five building tools list the five kinds in one order.
        // So arithmetic rather than a switch, as turnRight() does.
        // There is then no case a coverage gate cannot reach.
        // The static_asserts below hold the two orders together.
        return static_cast<BuildingKind>(
            static_cast<std::uint8_t>(tool) - 1);
    }

    static_assert(buildingFor(BuildTool::House) == BuildingKind::House);
    static_assert(
        buildingFor(BuildTool::FoodSource) == BuildingKind::FoodSource);
    static_assert(
        buildingFor(BuildTool::WaterSource) == BuildingKind::WaterSource);
    static_assert(
        buildingFor(BuildTool::FireStation) == BuildingKind::FireStation);
    static_assert(
        buildingFor(BuildTool::ArchitectPost)
        == BuildingKind::ArchitectPost);

    /**
     * @brief Get what a building of a kind stocks.
     *
     * A source stocks what it hands out; everything else eats, and eats
     * food.
     * One resource per building rather than a basket of them, because
     * Stock holds one and a second would be a second component.
     *
     * @param kind The kind of building.
     * @return The resource it holds.
     */
    [[nodiscard]] constexpr Resource stockedBy(BuildingKind kind) noexcept
    {
        if (kind == BuildingKind::WaterSource)
        {
            return Resource::Water;
        }

        return Resource::Food;
    }

    /**
     * @brief Get a building as it is the moment it is put up.
     *
     * A tenth of capacity, rounded down, so that something is delivered
     * to it before it runs out rather than after.
     *
     * @param kind The kind of building to put up.
     * @return The building's starting state.
     */
    [[nodiscard]] constexpr Building newlyBuilt(BuildingKind kind) noexcept
    {
        Stock stock{.resource = stockedBy(kind), .held = 0, .capacity = 100};
        stock.held = stock.capacity / 10;

        return Building{.kind = kind, .stock = stock};
    }

    /**
     * @brief Check whether two cells share an edge.
     *
     * It lives here rather than beside Cell because a building's
     * neighbourhood is the only thing that asks: a walker's next step is
     * nextFacing()'s business and never needs it.
     *
     * @param one The first cell.
     * @param other The second cell.
     * @return True when the two are one step apart, north, east, south or
     * west.  A cell is not adjacent to itself.
     */
    [[nodiscard]] constexpr bool orthogonallyAdjacent(
        Cell one, Cell other) noexcept
    {
        const auto dx = one.x > other.x ? one.x - other.x : other.x - one.x;
        const auto dy = one.y > other.y ? one.y - other.y : other.y - one.y;

        return dx + dy == 1;
    }

    /**
     * @brief Count a countdown down one tick, reloading it when it fires.
     *
     * The one shape all three of a building's counters share, so that
     * "every N ticks" is written once instead of three times.
     *
     * @param remaining The countdown, decremented in place.
     * @param period What to reload it with once it reaches zero.
     * @return True on the tick it comes due.
     */
    [[nodiscard]] constexpr bool due(
        std::int32_t &remaining, std::int32_t period) noexcept
    {
        --remaining;
        if (remaining > 0)
        {
            return false;
        }

        remaining = period;
        return true;
    }

} // namespace antwika::game
