#include "antwika/sudoku/BoardSink.hpp"

#include <cstdint>

#include <cstddef>
#include <string>

#include <antwika/replay/JsonShapes.hpp>
#include <antwika/replay/PayloadJson.hpp>

#include "antwika/sudoku/BoardFormatError.hpp"
#include "antwika/sudoku/Events.hpp"

namespace antwika::sudoku
{

    namespace
    {
        nlohmann::json newPuzzleSchema()
        {
            nlohmann::json schema = antwika::replay::documentShape(
                "sudoku.new_puzzle payload", {"cells"});
            schema["properties"]["cells"] = antwika::replay::wordShape();
            return schema;
        }

        // x and y are bounded by the grid rather than by their type.
        // A coordinate the board lacks is a payload nobody meant.
        // And the schema is where a payload's shape is judged.
        nlohmann::json setCellSchema()
        {
            nlohmann::json schema = antwika::replay::documentShape(
                "sudoku.set_cell payload", {"x", "y", "digit"});
            for (const char *field : {"x", "y"})
            {
                schema["properties"][field] =
                    antwika::replay::boundedCountShape(Board::kSize - 1);
            }
            schema["properties"]["digit"] =
                antwika::replay::boundedCountShape(Board::kSize);
            return schema;
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
                    antwika::replay::validatorFor<
                        newPuzzleSchema>(),
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
                    antwika::replay::validatorFor<
                        setCellSchema>(),
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
