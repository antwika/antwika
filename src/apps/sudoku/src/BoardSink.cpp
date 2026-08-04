#include "antwika/sudoku/BoardSink.hpp"

#include <cstdint>

#include <cstddef>
#include <string>

#include <nlohmann/json-schema.hpp>

#include <antwika/replay/PayloadJson.hpp>

#include "antwika/sudoku/BoardFormatError.hpp"
#include "antwika/sudoku/Events.hpp"

namespace antwika::sudoku
{

    namespace
    {
        nlohmann::json newPuzzleSchema()
        {
            nlohmann::json schema;
            schema["$schema"] = "http://json-schema.org/draft-07/schema#";
            schema["title"] = "sudoku.new_puzzle payload";
            schema["type"] = "object";
            schema["additionalProperties"] = false;
            schema["required"] = {"cells"}; // GCOVR_EXCL_LINE
            schema["properties"]["cells"]["type"] = "string";
            return schema;
        }

        // x and y are bounded by the grid rather than by their type.
        // A coordinate the board lacks is a payload nobody meant.
        // And the schema is where a payload's shape is judged.
        nlohmann::json setCellSchema()
        {
            nlohmann::json schema;
            schema["$schema"] = "http://json-schema.org/draft-07/schema#";
            schema["title"] = "sudoku.set_cell payload";
            schema["type"] = "object";
            schema["additionalProperties"] = false;
            schema["required"] = {"x", "y", "digit"}; // GCOVR_EXCL_LINE
            for (const char *field : {"x", "y"})
            {
                schema["properties"][field]["type"] = "integer";
                schema["properties"][field]["minimum"] = 0;
                schema["properties"][field]["maximum"] =
                    Board::kSize - 1;
            }
            schema["properties"]["digit"]["type"] = "integer";
            schema["properties"]["digit"]["minimum"] = 0;
            schema["properties"]["digit"]["maximum"] = Board::kSize;
            return schema;
        }

        const nlohmann::json_schema::json_validator &
        newPuzzleValidator()
        {
            static const nlohmann::json_schema::json_validator validator(
                newPuzzleSchema()); // GCOVR_EXCL_LINE
            return validator;
        }

        const nlohmann::json_schema::json_validator &setCellValidator()
        {
            static const nlohmann::json_schema::json_validator validator(
                setCellSchema()); // GCOVR_EXCL_LINE
            return validator;
        }
    } // namespace

    BoardSink::BoardSink(
        PuzzleState &state, std::uint64_t solveStepBudget)
        : state(state), solveStepBudget(solveStepBudget)
    {
    }

    void BoardSink::handle(const TickEvent &event)
    {
        if (event.event.name == events::kNewPuzzle)
        {
            const auto parsed =
                antwika::replay::parseAndValidatePayload<
                    BoardFormatError>(
                    event.event.payload,
                    newPuzzleValidator(),
                    "BoardSink: sudoku.new_puzzle payload");

            state.start(
                Board::parse(parsed.at("cells").get<std::string>()));
            return;
        }

        if (event.event.name == events::kSetCell)
        {
            const auto parsed =
                antwika::replay::parseAndValidatePayload<
                    BoardFormatError>(
                    event.event.payload,
                    setCellValidator(),
                    "BoardSink: sudoku.set_cell payload");

            state.write(
                Square{
                    .row = parsed.at("y").get<std::size_t>(),
                    .col = parsed.at("x").get<std::size_t>()},
                parsed.at("digit").get<int>());
            return;
        }

        if (event.event.name == events::kSolve)
        {
            state.solve({.maxSteps = solveStepBudget});
        }
    }

} // namespace antwika::sudoku
