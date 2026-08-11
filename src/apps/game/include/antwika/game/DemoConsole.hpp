#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <antwika/console/ConsoleState.hpp>
#include <antwika/console/IConsoleCommands.hpp>
#include <antwika/geometry/Grid.hpp>
#include <antwika/log/ILogger.hpp>
#include <antwika/tilemap/Column.hpp>
#include <antwika/tilemap/TileMap.hpp>

namespace antwika::game
{

    struct Player final
    {
        geometry::GridCell cell{.column = 2, .row = 4};
        std::int32_t level = 0;
        std::int32_t direction = 0;
        std::uint32_t moveTicks = 0;
    };

    /**
     * @brief Whether the level offers a walkable standing surface.
     *
     * @param column The column holding the candidate slab.
     * @param level The level of the slab to stand on.
     * @return True when a slab sits at the level, it is not a wall,
     *         it is not unbridged water, and no slab blocks the
     *         clearance above it.
     */
    [[nodiscard]] bool standableWalkable(
        const tilemap::Column &column, std::int32_t level);

    /**
     * @brief The highest standable walkable surface of a column.
     *
     * @return Its level, or nothing when the column offers none.
     */
    [[nodiscard]] std::optional<std::int32_t> topStandableWalkable(
        const tilemap::Column &column);

    /**
     * @brief The level a landing player rests at in a column.
     *
     * Ensures: the top standable walkable surface wins, else the
     *          top slab's level, else zero for an empty column.
     */
    [[nodiscard]] std::int32_t restingLevel(
        const tilemap::Column &column);

    /**
     * @brief The level a step into a neighbour cell lands on.
     *
     * @param map The map both cells lie in.
     * @param from The cell the player stands in.
     * @param fromLevel The level the player stands on.
     * @param to The cell the player steps into.
     * @return The landing level, or nothing when the step is
     *         blocked.
     *
     * Ensures: the target's top surface at or below the player is
     *          eligible when standable walkable, one level up is
     *          eligible only via a stair on either end, and the
     *          highest eligible level wins.
     */
    [[nodiscard]] std::optional<std::int32_t> landingLevel(
        const tilemap::TileMap &map,
        geometry::GridCell from,
        std::int32_t fromLevel,
        geometry::GridCell to);

    class DemoCommands final : public console::IConsoleCommands
    {
    public:
        DemoCommands(
            tilemap::TileMap &map,
            Player &player,
            log::ILogger &logger);

        DemoCommands(const DemoCommands &) = delete;
        DemoCommands(DemoCommands &&) = delete;

        DemoCommands &operator=(const DemoCommands &) = delete;
        DemoCommands &operator=(DemoCommands &&) = delete;

        void execute(
            const std::string &command,
            console::ConsoleState &console) override;

        [[nodiscard]] std::vector<std::string> names() const override;

    private:
        void loadMap(
            const std::string &arguments,
            console::ConsoleState &console);

        void teleport(
            const std::string &arguments,
            console::ConsoleState &console);

        void setPalette(
            const std::string &arguments,
            console::ConsoleState &console);

        tilemap::TileMap &map;
        Player &player;
        log::ILogger &logger;
    };

}
