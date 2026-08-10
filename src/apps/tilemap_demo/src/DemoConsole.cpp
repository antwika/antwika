#include "antwika/tilemap_demo/DemoConsole.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
#include <optional>
#include <string_view>
#include <utility>

#include <antwika/log/Level.hpp>
#include <antwika/tilemap/MapFile.hpp>
#include <antwika/tilemap/Rgb.hpp>
#include <antwika/tilemap/TerrainClass.hpp>
#include <antwika/tilemap/TileMapError.hpp>

namespace antwika::tilemap_demo
{

    namespace
    {
        using antwika::geometry::GridCell;
        using antwika::tilemap::Rgb;
        using antwika::tilemap::TileMap;

        struct Split final
        {
            std::string word;
            std::string rest;
        };

        [[nodiscard]] Split splitFirst(const std::string &line)
        {
            const auto space = line.find(' ');

            Split split;

            split.word = line.substr(0, space);

            if (space != std::string::npos)
            {
                const auto rest = line.substr(space + 1);
                const auto first = rest.find_first_not_of(' ');

                if (first != std::string::npos)
                {
                    const auto last = rest.find_last_not_of(' ');

                    split.rest =
                        rest.substr(first, last - first + 1);
                }
            }

            return split;
        }

        [[nodiscard]] std::optional<std::uint32_t> parseNumber(
            const std::string &text)
        {
            std::uint32_t value = 0;
            const auto *end = text.data() + text.size();
            const auto result =
                std::from_chars(text.data(), end, value);

            if (result.ec != std::errc{} || result.ptr != end)
            {
                return std::nullopt;
            }

            return value;
        }

        [[nodiscard]] std::optional<std::uint32_t> hexNibble(
            const char digit)
        {
            if (digit >= '0' && digit <= '9')
            {
                return static_cast<std::uint32_t>(digit - '0');
            }

            if (digit >= 'a' && digit <= 'f')
            {
                return static_cast<std::uint32_t>(digit - 'a') + 10;
            }

            if (digit >= 'A' && digit <= 'F')
            {
                return static_cast<std::uint32_t>(digit - 'A') + 10;
            }

            return std::nullopt;
        }

        [[nodiscard]] std::optional<Rgb> parseHexColor(
            const std::string &text)
        {
            const std::size_t first =
                !text.empty() && text.front() == '#' ? 1 : 0;

            if (text.size() != first + 6)
            {
                return std::nullopt;
            }

            std::array<std::uint8_t, 3> channels{};

            for (std::size_t channel = 0;
                 channel < channels.size();
                 ++channel)
            {
                const auto high =
                    hexNibble(text[first + channel * 2]);
                const auto low =
                    hexNibble(text[first + 1 + channel * 2]);

                if (!high.has_value() || !low.has_value())
                {
                    return std::nullopt;
                }

                channels[channel] = static_cast<std::uint8_t>(
                    (*high << 4U) | *low);
            }

            return Rgb{
                .red = channels[0],
                .green = channels[1],
                .blue = channels[2]};
        }

        [[nodiscard]] TileMap withPalette(
            const TileMap &map, const Rgb ink, const Rgb paper)
        {
            auto header = map.header();

            header.ink = ink;
            header.paper = paper;

            TileMap rebuilt(
                std::move(header), map.columns(), map.rows());

            for (std::uint32_t row = 0; row < map.rows(); ++row)
            {
                for (std::uint32_t column = 0;
                     column < map.columns();
                     ++column)
                {
                    const auto cell =
                        GridCell{.column = column, .row = row};

                    rebuilt.at(cell) = map.at(cell);
                }
            }

            for (const auto &entity : map.entities())
            {
                rebuilt.addEntity(entity);
            }

            return rebuilt;
        }

        constexpr std::array<std::string_view, 5> kHelpLines{
            "help - list the commands",
            "map <path> - load a map file",
            "tp <column> <row> - teleport the player",
            "pos - print the player position",
            "palette <ink|paper> <#rrggbb> - recolor the map"};
    }

    bool walkable(const TileMap &map, const GridCell cell)
    {
        const auto terrain = map.at(cell).terrain;

        return terrain != tilemap::TerrainClass::Wall
               && terrain != tilemap::TerrainClass::Water;
    }

    DemoCommands::DemoCommands(
        TileMap &map, Player &player, log::ILogger &logger)
        : map(map), player(player), logger(logger)
    {
    }

    void DemoCommands::execute(
        const std::string &command,
        console::ConsoleState &console)
    {
        const auto asked = splitFirst(command);

        if (asked.word == "help")
        {
            for (const auto &line : kHelpLines)
            {
                console.pushHistory(std::string(line));
            }

            console.pushHistory("quit - close the demo");
            return;
        }

        if (asked.word == "map")
        {
            loadMap(asked.rest, console);
            return;
        }

        if (asked.word == "tp")
        {
            teleport(asked.rest, console);
            return;
        }

        if (asked.word == "pos")
        {
            console.pushHistory(
                "player at "
                + std::to_string(player.cell.column) + ","
                + std::to_string(player.cell.row)
                + " h=" + std::to_string(player.height));
            return;
        }

        if (asked.word == "palette")
        {
            setPalette(asked.rest, console);
            return;
        }

        console.pushHistory(
            "unknown command: " + asked.word + " - try help");
    }

    std::vector<std::string> DemoCommands::names() const
    {
        return {"help", "map", "tp", "pos", "palette"};
    }

    void DemoCommands::loadMap(
        const std::string &arguments,
        console::ConsoleState &console)
    {
        if (arguments.empty())
        {
            console.pushHistory("map: name a file to load");
            return;
        }

        try
        {
            map = tilemap::loadMapFile(arguments);
        }
        catch (const tilemap::TileMapError &error)
        {
            console.pushHistory("map: " + std::string(error.what()));
            return;
        }

        player.cell.column =
            std::min(player.cell.column, map.columns() - 1);
        player.cell.row = std::min(player.cell.row, map.rows() - 1);
        player.height = map.at(player.cell).height;
        player.moveTicks = 0;

        console.pushHistory("loaded " + arguments);
        logger.log(log::Level::Info, "console loaded " + arguments);
    }

    void DemoCommands::teleport(
        const std::string &arguments,
        console::ConsoleState &console)
    {
        const auto split = splitFirst(arguments);
        const auto column = parseNumber(split.word);
        const auto row = parseNumber(split.rest);

        if (!column.has_value() || !row.has_value())
        {
            console.pushHistory("tp: say tp <column> <row>");
            return;
        }

        const auto target = GridCell{
            .column = std::min(*column, map.columns() - 1),
            .row = std::min(*row, map.rows() - 1)};

        if (!walkable(map, target))
        {
            console.pushHistory(
                "tp: " + std::to_string(target.column) + ","
                + std::to_string(target.row)
                + " is not walkable");
            return;
        }

        player.cell = target;
        player.height = map.at(target).height;
        player.moveTicks = 0;

        console.pushHistory(
            "teleported to " + std::to_string(target.column) + ","
            + std::to_string(target.row));
    }

    void DemoCommands::setPalette(
        const std::string &arguments,
        console::ConsoleState &console)
    {
        const auto split = splitFirst(arguments);
        const auto color = parseHexColor(split.rest);

        const bool ink = split.word == "ink";
        const bool paper = split.word == "paper";

        if ((!ink && !paper) || !color.has_value())
        {
            console.pushHistory(
                "palette: say palette <ink|paper> <#rrggbb>");
            return;
        }

        map = withPalette(
            map,
            ink ? *color : map.header().ink,
            paper ? *color : map.header().paper);

        console.pushHistory(
            "palette " + split.word + " set to " + split.rest);
    }

}
