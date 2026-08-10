#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <antwika/console/ConsoleState.hpp>
#include <antwika/console/IConsoleCommands.hpp>
#include <antwika/geometry/Grid.hpp>
#include <antwika/log/ILogger.hpp>
#include <antwika/tilemap/TileMap.hpp>

namespace antwika::tilemap_demo
{

    struct Player final
    {
        geometry::GridCell cell{.column = 2, .row = 4};
        std::int32_t height = 0;
        std::int32_t direction = 0;
        std::uint32_t moveTicks = 0;
    };

    /**
     * @brief Whether the player may stand on the given cell.
     *
     * Requires: the cell lies inside the map bounds.
     */
    [[nodiscard]] bool walkable(
        const tilemap::TileMap &map, geometry::GridCell cell);

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
