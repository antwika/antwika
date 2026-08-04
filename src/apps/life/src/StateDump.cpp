#include "antwika/life/StateDump.hpp"

#include <cstddef>
#include <exception>
#include <string>
#include <utility>

#include <nlohmann/json-schema.hpp>

namespace antwika::life
{

    namespace
    {
        [[nodiscard]] nlohmann::json coordinateSchema(
            std::int64_t minimum, std::int64_t maximum)
        {
            nlohmann::json schema;
            schema["type"] = "object";
            schema["additionalProperties"] = false;
            schema["required"] = {"x", "y"}; // GCOVR_EXCL_LINE
            schema["properties"]["x"]["type"] = "integer";
            schema["properties"]["x"]["minimum"] = minimum;
            schema["properties"]["x"]["maximum"] = maximum;
            schema["properties"]["y"]["type"] = "integer";
            schema["properties"]["y"]["minimum"] = minimum;
            schema["properties"]["y"]["maximum"] = maximum;
            return schema;
        }

        nlohmann::json stateSchema()
        {
            constexpr std::int64_t kCellMax = 0xFFFFFFFF;
            constexpr std::int64_t kPixelMin = -2147483648;
            constexpr std::int64_t kPixelMax = 2147483647;

            nlohmann::json schema;
            schema["$schema"] = "http://json-schema.org/draft-07/schema#";
            schema["title"] = "antwika life dump state";
            schema["type"] = "object";
            schema["additionalProperties"] = false;
            schema["required"] = {
                "board", "dragging", "visited"}; // GCOVR_EXCL_LINE

            auto &board = schema["properties"]["board"];
            board["type"] = "object";
            board["additionalProperties"] = false;
            board["required"] = {
                "width", "height", "cells"}; // GCOVR_EXCL_LINE
            board["properties"]["width"]["type"] = "integer";
            board["properties"]["width"]["minimum"] = 0;
            board["properties"]["width"]["maximum"] = kCellMax;
            board["properties"]["height"]["type"] = "integer";
            board["properties"]["height"]["minimum"] = 0;
            board["properties"]["height"]["maximum"] = kCellMax;
            board["properties"]["cells"]["type"] = "string";

            schema["properties"]["dragging"]["type"] = "boolean";
            schema["properties"]["visited"]["type"] = "array";
            schema["properties"]["visited"]["items"] =
                coordinateSchema(0, kCellMax);
            schema["properties"]["lastDrag"] =
                coordinateSchema(kPixelMin, kPixelMax);
            return schema;
        }

        const nlohmann::json_schema::json_validator &stateValidator()
        {
            // The excluded closing line carries the static guard.
            // Its concurrency arms are unreachable one-threaded.
            // See docs/confirming-unreachable-branches.md.
            static const nlohmann::json_schema::json_validator validator(
                stateSchema()); // GCOVR_EXCL_LINE
            return validator;
        }
    } // namespace

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
            // The excluded line is the pair temporaries' unwind arms.
            // Only a failed allocation inside them could take one.
            // See docs/confirming-unreachable-branches.md.
            encoded["visited"].push_back(
                // GCOVR_EXCL_START
                nlohmann::json{{"x", cell.x}, {"y", cell.y}});
            // GCOVR_EXCL_STOP
        }

        // Absent means no drag was under way.
        // A member for it would be a place for no position.
        if (dump.lastDrag.has_value())
        {
            encoded["lastDrag"]["x"] = dump.lastDrag->x;
            encoded["lastDrag"]["y"] = dump.lastDrag->y;
        }

        return encoded;

        // gcov puts the returned value's unwind block here.
        // Nothing after the last throwable call throws.
        // So no input reaches it.
    } // GCOVR_EXCL_LINE

    StateDump stateDumpFromJson(const nlohmann::json &state)
    {
        try
        {
            stateValidator().validate(state);
        }
        // The validator's failure type is the library's business.
        // What this format promises is StateDumpError.
        // So it is rewrapped here, as apps/game's decoder rewraps.
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

        // gcov puts the returned value's unwind block here.
        // Nothing between the last throw and the return throws.
    } // GCOVR_EXCL_LINE

} // namespace antwika::life
