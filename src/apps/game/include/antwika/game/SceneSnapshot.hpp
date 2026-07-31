#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include <antwika/ecs/World.hpp>

#include "antwika/game/BuildGhost.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Direction.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/PathIndex.hpp"

namespace antwika::game
{

    using antwika::ecs::World;

    /**
     * @brief One walker, as state that outlives a frame needs to know it.
     *
     * What a summary reports and a save file holds, so it carries where a
     * walker is and nothing about how it is being shown getting there.
     * WalkerSprite is the picture's answer to the same walker.
     */
    struct WalkerView
    {
        Cell at;
        Direction facing = Direction::East;

        /**
         * @brief Compare two walker views.
         * @param other The view to compare against.
         * @return True when both the cell and the facing match.
         */
        [[nodiscard]] bool operator==(const WalkerView &other) const = default;
    };

    /**
     * @brief One walker, as a frame needs to know it.
     *
     * Separate from WalkerView because a walker part of the way between
     * two cells is a fact about the picture and not about the state.
     * GameSummary and SaveGame both hold WalkerViews, so folding the two
     * render-side fields into that type would put them in a persisted
     * schema and in the value a replay determinism test compares -- which
     * is the same reason which of sixteen tiles a road shows is worked
     * out in GridScene and kept out of the snapshot entirely.
     */
    struct WalkerSprite
    {
        Cell at;
        Direction facing = Direction::East;

        /** @brief The cell being stepped out of, if there is one. */
        std::optional<Cell> from{};

        /**
         * @brief How many whole ticks of this step have gone.
         *
         * Counted up rather than down, since that is the direction a
         * fraction of the way there runs; Walker counts the same span
         * the other way because what it needs to know is when to move.
         */
        std::uint8_t ticksIntoStep = 0;

        /**
         * @brief Compare two walker sprites.
         * @param other The sprite to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(
            const WalkerSprite &other) const = default;
    };

    /**
     * @brief One building, as a frame needs to know it.
     */
    struct BuildingView
    {
        Cell at;
        BuildingKind kind = BuildingKind::House;

        /**
         * @brief Compare two building views.
         * @param other The view to compare against.
         * @return True when both the cell and the kind match.
         */
        [[nodiscard]] bool operator==(
            const BuildingView &other) const = default;
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
        std::vector<WalkerSprite> walkers;
        std::vector<BuildingView> buildings;

        /**
         * @brief Where the selected tool would land if it were clicked.
         *
         * The one member snapshotOf() does not fill in: it is a picture
         * worked out on the render side from a channel no replay
         * reproduces, so nothing about it may be taken from the World --
         * see BuildGhost. Invisible by default, so a snapshot nobody has
         * given one draws none.
         */
        BuildGhost ghost;

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
     * @param world Read for the walkers, as of its last commit().
     * @param paths Read for the path cells.
     * @param camera The camera to draw through.
     * @param extent The bounds to draw within.
     * @return The frame's description, with no ghost; whoever draws
     * fills that in from ghostFor().
     */
    [[nodiscard]] SceneSnapshot snapshotOf(
        const World &world,
        const PathIndex &paths,
        const Camera &camera,
        GridExtent extent);

    /**
     * @brief List every walker as state rather than as a picture.
     *
     * What a summary and a save file want, so neither has to take a whole
     * frame's worth of camera, paths and buildings to get at the walkers,
     * and neither picks up the two fields that only exist to draw with.
     *
     * @param world Read for the walkers, as of its last commit().
     * @return One view per walker, in the world's own order.
     */
    [[nodiscard]] std::vector<WalkerView> walkerViewsOf(const World &world);

} // namespace antwika::game
