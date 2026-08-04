#include "antwika/sudoku/SnapshotStore.hpp"

#include <array>
#include <cstddef>
#include <exception>

#include <nlohmann/json-schema.hpp>

#include <antwika/console/SnapshotError.hpp>
#include <antwika/replay/MigrationChain.hpp>

#include "antwika/sudoku/BoardFormatError.hpp"

namespace antwika::sudoku
{

    using antwika::console::SnapshotError;

    namespace
    {
        // The names a dump document holds, one per status.
        // Persisted, so they may not change once written.
        constexpr std::array<std::string_view, kStatusCount>
            kStatusNames{
                "playing",
                "solved",
                "complete",
                "unsolvable",
                "limit_exceeded",
                "given_locked"};

        nlohmann::json stateSchema()
        {
            nlohmann::json schema;
            schema["$schema"] =
                "http://json-schema.org/draft-07/schema#";
            schema["title"] = "antwika sudoku dump state";
            schema["type"] = "object";
            schema["additionalProperties"] = false;
            schema["required"] = {
                "givens", "cells", "note"}; // GCOVR_EXCL_LINE
            schema["properties"]["givens"]["type"] = "string";
            schema["properties"]["cells"]["type"] = "string";
            schema["properties"]["note"]["type"] = "string";

            // The square's bounds live here rather than in a check.
            // A row outside the grid then refuses as a shape would.
            auto &chosen = schema["properties"]["chosen"];
            chosen["type"] = "object";
            chosen["additionalProperties"] = false;
            chosen["required"] = {"row", "col"}; // GCOVR_EXCL_LINE
            for (const auto *member : {"row", "col"})
            {
                chosen["properties"][member]["type"] = "integer";
                chosen["properties"][member]["minimum"] = 0;
                chosen["properties"][member]["maximum"] =
                    Board::kSize - 1;
            }

            return schema;
        }

        const nlohmann::json_schema::json_validator &stateValidator()
        {
            // The excluded closing line carries the static guard.
            // Its concurrency arms are unreachable one-threaded.
            // See docs/confirming-unreachable-branches.md.
            static const nlohmann::json_schema::json_validator
                validator(stateSchema()); // GCOVR_EXCL_LINE
            return validator;
        }

        // Version 1 is the first shape ever written.
        // So there is nothing yet for a chain to bring up.
        [[nodiscard]] antwika::replay::MigrationChain
        noStateDumpMigrations()
        {
            return antwika::replay::MigrationChain(
                {}, kStateDumpVersion);
        }
    } // namespace

    std::string_view statusName(const Status status) noexcept
    {
        return kStatusNames[
            static_cast<std::size_t>(status) % kStatusCount];
    }

    std::optional<Status> statusFromName(
        const std::string_view name) noexcept
    {
        for (std::size_t index = 0; index < kStatusCount; ++index)
        {
            if (kStatusNames[index] == name)
            {
                return static_cast<Status>(index);
            }
        }

        return std::nullopt;
    }

    nlohmann::json puzzleStateToJson(const PuzzleState &state)
    {
        nlohmann::json encoded;

        encoded["givens"] = state.clues().format();
        encoded["cells"] = state.board().format();

        // Absent means nothing was picked.
        // A member for it would be a name for no square.
        if (state.selected().has_value())
        {
            encoded["chosen"]["row"] = state.selected()->row;
            encoded["chosen"]["col"] = state.selected()->col;
        }

        encoded["note"] = std::string(statusName(state.status()));

        return encoded;

        // gcov puts the returned value's unwind block here.
        // Nothing between its construction and the return throws.
    } // GCOVR_EXCL_LINE

    void puzzleStateFromJson(
        const nlohmann::json &state, PuzzleState &into)
    {
        try
        {
            stateValidator().validate(state);
        }
        // The validator's failure type is the library's business.
        // What this seam promises is console::SnapshotError.
        // So it is rewrapped here, as game's stateDumpFromJson does.
        catch (const std::exception &failed) // GCOVR_EXCL_LINE
        {
            throw SnapshotError(
                std::string(
                    "antwika::sudoku: dump state failed schema "
                    "validation: ")
                + failed.what());
        }

        Board givens;
        Board cells;

        try
        {
            givens = Board::parse(
                state.at("givens").get<std::string>());
            cells = Board::parse(state.at("cells").get<std::string>());
        }
        // The board's own parser promises BoardFormatError.
        // Rewrapped at this seam for the validator's exact reason.
        catch (const BoardFormatError &failed) // GCOVR_EXCL_LINE
        {
            throw SnapshotError(
                std::string(
                    "antwika::sudoku: dump holds a board that does "
                    "not parse: ")
                + failed.what());
        }

        const auto named = state.at("note").get<std::string>();
        const auto note = statusFromName(named);

        if (!note.has_value())
        {
            throw SnapshotError(
                "antwika::sudoku: dump names a status this build "
                "does not know: "
                + named);
        }

        std::optional<Square> chosen;

        if (state.contains("chosen"))
        {
            chosen = Square{
                .row = state.at("chosen").at("row")
                           .get<std::size_t>(),
                .col = state.at("chosen").at("col")
                           .get<std::size_t>()};
        }

        into.restore(givens, cells, chosen, *note);
    }

    SudokuSnapshotStore::SudokuSnapshotStore(
        PuzzleState &state) noexcept
        : antwika::console::JsonSnapshotStore<
              antwika::console::SnapshotError>(
              {.magic = kStateDumpMagic,
               .version = kStateDumpVersion},
              "antwika sudoku state dump document",
              noStateDumpMigrations),
          state(state)
    {
    }

    nlohmann::json SudokuSnapshotStore::takeState(const std::string &)
    {
        return puzzleStateToJson(state);
    }

    void SudokuSnapshotStore::applyState(
        const std::string &, const nlohmann::json &dumped)
    {
        puzzleStateFromJson(dumped, state);
    }

} // namespace antwika::sudoku
