#include "antwika/life/StateDump.hpp"

#include <cstddef>
#include <exception>
#include <string>
#include <utility>

#include <antwika/replay/JsonShapes.hpp>

namespace antwika::life
{

    namespace
    {
        [[nodiscard]] nlohmann::json coordinateSchema(
            nlohmann::json bounds)
        {
            nlohmann::json schema =
                antwika::replay::objectShape({"x", "y"});
            schema["properties"]["x"] = bounds;
            schema["properties"]["y"] = std::move(bounds);
            return schema;
        } // GCOVR_EXCL_LINE

        nlohmann::json stateSchema()
        {
            constexpr std::int64_t kCellMax = 0xFFFFFFFF;

            nlohmann::json schema = antwika::replay::documentShape(
                "antwika life dump state",
                {"board", "dragging", "visited"});

            auto &board = schema["properties"]["board"];
            board = antwika::replay::objectShape(
                {"width", "height", "cells"});
            board["properties"]["width"] =
                antwika::replay::boundedCountShape(kCellMax);
            board["properties"]["height"] =
                antwika::replay::boundedCountShape(kCellMax);
            board["properties"]["cells"] = antwika::replay::wordShape();

            schema["properties"]["dragging"]["type"] = "boolean";
            schema["properties"]["visited"]["type"] = "array";
            schema["properties"]["visited"]["items"] = coordinateSchema(
                antwika::replay::boundedCountShape(kCellMax));
            schema["properties"]["lastDrag"] =
                coordinateSchema(antwika::replay::coordinateShape());
            return schema;
        } // GCOVR_EXCL_LINE
    }

    antwika::replay::MigrationChain standardStateDumpMigrations()
    {
        return antwika::replay::MigrationChain(
            {}, kStateDumpVersion); // GCOVR_EXCL_LINE
    }

    nlohmann::json stateDumpToJson(const StateDump &dump)
    {
        nlohmann::json encoded;

        std::string cells;
        cells.reserve(dump.board.alive.size());

        for (const bool alive : dump.board.alive)
        {
            cells.push_back(alive ? '1' : '0');
        }

        encoded["board"]["width"] = dump.board.width;
        encoded["board"]["height"] = dump.board.height;
        encoded["board"]["cells"] = std::move(cells);
        encoded["dragging"] = dump.dragging;

        encoded["visited"] = nlohmann::json::array();

        for (const auto &cell : dump.visited)
        {
            encoded["visited"].push_back(
                // GCOVR_EXCL_START
                nlohmann::json{{"x", cell.x}, {"y", cell.y}});
            // GCOVR_EXCL_STOP
        }

        if (dump.lastDrag.has_value())
        {
            encoded["lastDrag"]["x"] = dump.lastDrag->x;
            encoded["lastDrag"]["y"] = dump.lastDrag->y;
        }

        return encoded;

    } // GCOVR_EXCL_LINE

    StateDump stateDumpFromJson(const nlohmann::json &state)
    {
        try
        {
            antwika::replay::validatorFor<stateSchema>().validate(state);
        }
        catch (const std::exception &failed) // GCOVR_EXCL_LINE
        {
            throw StateDumpError(
                std::string(
                    "antwika::life: dump state failed schema "
                    "validation: ")
                + failed.what());
        }

        StateDump dump;

        const auto &board = state.at("board");
        dump.board.width = board.at("width").get<std::uint32_t>();
        dump.board.height = board.at("height").get<std::uint32_t>();

        const auto cells = board.at("cells").get<std::string>();
        const auto expected = std::uint64_t{dump.board.width}
                              * std::uint64_t{dump.board.height};

        if (cells.size() != expected)
        {
            throw StateDumpError(
                "antwika::life: dump cell string is "
                + std::to_string(cells.size())
                + " characters for a board of "
                + std::to_string(expected));
        }

        dump.board.alive.reserve(cells.size());

        for (const char cell : cells)
        {
            if (cell != '0' && cell != '1')
            {
                throw StateDumpError(
                    std::string(
                        "antwika::life: dump cell string holds a "
                        "character that is not '0' or '1': ")
                    + cell);
            }

            dump.board.alive.push_back(cell == '1');
        }

        dump.dragging = state.at("dragging").get<bool>();

        for (const auto &visited : state.at("visited"))
        {
            const CellCoordinate cell{
                .x = visited.at("x").get<std::uint32_t>(),
                .y = visited.at("y").get<std::uint32_t>()};

            if (cell.x >= dump.board.width
                || cell.y >= dump.board.height)
            {
                throw StateDumpError(
                    "antwika::life: dump visits a cell off its own "
                    "board: ("
                    + std::to_string(cell.x) + ", "
                    + std::to_string(cell.y) + ")");
            }

            dump.visited.push_back(cell);
        }

        if (state.contains("lastDrag"))
        {
            dump.lastDrag = antwika::input::Position{
                .x = state.at("lastDrag").at("x").get<std::int32_t>(),
                .y = state.at("lastDrag").at("y").get<std::int32_t>()};
        }

        return dump;

    } // GCOVR_EXCL_LINE

}
