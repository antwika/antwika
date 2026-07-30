#pragma once

#include <vector>

#include <antwika/ecs/World.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Direction.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/Walker.hpp"

namespace antwika::game
{

    using antwika::ecs::World;

    /**
     * @brief One walker, as a frame needs to know it.
     */
    struct WalkerView
    {
        Cell at;
        Direction facing = Direction::East;
        WalkerKind kind = WalkerKind::Food;
        std::int32_t carried = 0;

        /**
         * @brief How many cells this walker has moved since it appeared.
         *
         * Carried into the picture because it is the walker's own
         * elapsed tick count, and antwika::animation keeps no time of
         * its own: a clip is resolved against a number the caller has
         * already got.  A walker takes exactly one step per tick while
         * it can move, so this counts the ticks it has been walking --
         * and a walker with nowhere to go holds it, which is what makes
         * one standing still stop animating rather than march on the
         * spot.
         *
         * It is simulation state a replay already reproduces, so the
         * picture stays a function of the recording without a frame
         * number ever being written to one.
         */
        std::int32_t stepsTaken = 0;

        /**
         * @brief Compare two walker views.
         * @param other The view to compare against.
         * @return True when the cell, the facing, the kind, the load and
         * the distance walked all match.
         */
        [[nodiscard]] bool operator==(const WalkerView &other) const = default;
    };

    /**
     * @brief One building, as a frame needs to know it.
     *
     * What it holds and what it can hold, and not the risks or the
     * countdowns: a frame draws a bar, and everything else about a
     * building is simulation state a picture has no use for.
     */
    struct BuildingView
    {
        Cell at;
        BuildingKind kind = BuildingKind::House;
        std::int32_t held = 0;
        std::int32_t capacity = 100;

        /**
         * @brief Compare two building views.
         * @param other The view to compare against.
         * @return True when the cell, the kind and the stock all match.
         */
        [[nodiscard]] bool operator==(const BuildingView &other) const
            = default;
    };

    /**
     * @brief Everything one frame needs, and nothing that can change under
     * it.
     *
     * A plain value taken from the World rather than a reference into it,
     * for the reason apps/poker's TableSnapshot exists: it lets GridScene
     * be a pure function of its argument, so a picture can be asserted
     * call by call against a mock renderer instead of having to be looked
     * at.
     */
    struct SceneSnapshot
    {
        Camera camera;
        GridExtent extent;
        std::vector<Cell> paths;
        std::vector<WalkerView> walkers;

        /**
         * @brief Every building, ascending by cell.
         *
         * Ordered like paths are, so a scene may binary-search it rather
         * than build a second index of its own -- and so that two runs
         * that put the same buildings up draw them in the same order
         * whatever order the entities were created in.
         */
        std::vector<BuildingView> buildings;

        /**
         * @brief Compare two snapshots.
         * @param other The snapshot to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(
            const SceneSnapshot &other) const = default;
    };

    /**
     * @brief Take a snapshot of what to draw.
     *
     * Paths come from the index rather than from a view over the world,
     * because the index is already ordered and a view is not ordered by
     * anything a reader can name.
     *
     * Buildings do come from a view over the world, and are sorted after
     * the fact rather than read from an index: BuildingIndex holds cells
     * and not the stock a frame needs, so a second pass over the world
     * would be the price of an order the sort gives directly.
     *
     * @param world Read for the walkers and the buildings, as of its last
     * commit().
     * @param paths Read for the path cells.
     * @param camera The camera to draw through.
     * @param extent The bounds to draw within.
     * @return The frame's description.
     */
    [[nodiscard]] SceneSnapshot snapshotOf(
        const World &world,
        const PathIndex &paths,
        const Camera &camera,
        GridExtent extent);

} // namespace antwika::game
