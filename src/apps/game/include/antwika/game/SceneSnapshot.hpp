#pragma once

#include <vector>

#include <antwika/ecs/World.hpp>

#include "antwika/game/BuildGhost.hpp"
#include "antwika/game/BuildTool.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Direction.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/PathIndex.hpp"

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

        /**
         * @brief Compare two walker views.
         * @param other The view to compare against.
         * @return True when both the cell and the facing match.
         */
        [[nodiscard]] bool operator==(const WalkerView &other) const = default;
    };

    /**
     * @brief One building, as a frame needs to know it.
     */
    struct BuildingView
    {
        Cell at;
        BuildTool kind = BuildTool::House;

        /**
         * @brief Compare two building views.
         * @param other The view to compare against.
         * @return True when both the cell and the tool match.
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
        std::vector<WalkerView> walkers;
        std::vector<BuildingView> buildings;

        /**
         * @brief Where the selected tool would land if it were clicked.
         *
         * A picture rather than a fact about the world, but one only the
         * simulation can work out, since which cell a pixel means is a
         * function of the camera -- see BuildGhost. Invisible by default,
         * so a snapshot of a world nobody has pointed at draws none.
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
     * @return The frame's description.
     */
    [[nodiscard]] SceneSnapshot snapshotOf(
        const World &world,
        const PathIndex &paths,
        const Camera &camera,
        GridExtent extent);

} // namespace antwika::game
