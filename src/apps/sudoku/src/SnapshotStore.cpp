#include "antwika/sudoku/SnapshotStore.hpp"

#include <cstddef>
#include <exception>

#include <antwika/console/SnapshotError.hpp>
#include <antwika/enums/FromName.hpp>
#include <antwika/enums/NameTable.hpp>
#include <antwika/replay/JsonShapes.hpp>
#include <antwika/replay/MigrationChain.hpp>

#include "antwika/sudoku/BoardFormatError.hpp"

namespace antwika::sudoku
{

    using antwika::console::SnapshotError;

    namespace
    {
        constexpr antwika::enums::NameTable<Status> kStatusNames{{
            "playing",
            "solved",
            "complete",
            "unsolvable",
            "limit_exceeded",
            "given_locked"}};

        nlohmann::json squareShape()
        {
            nlohmann::json shape =
                antwika::replay::objectShape({"row", "col"});

            for (const auto *member : {"row", "col"})
            {
                shape["properties"][member] =
                    antwika::replay::boundedCountShape(Board::kSize - 1);
            }

            return shape;
        } // GCOVR_EXCL_LINE

        nlohmann::json stateSchema()
        {
            nlohmann::json schema = antwika::replay::documentShape(
                "antwika sudoku dump state",
                {"givens", "cells", "note"});

            schema["properties"]["givens"] = antwika::replay::wordShape();
            schema["properties"]["cells"] = antwika::replay::wordShape();
            schema["properties"]["note"] = antwika::replay::wordShape();
            schema["properties"]["chosen"] = squareShape();

            return schema;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] antwika::replay::MigrationChain
        noStateDumpMigrations()
        {
            return antwika::replay::MigrationChain(
                {}, kStateDumpVersion);
        }
    }

    std::string_view statusName(const Status status) noexcept
    {
        return kStatusNames.name(status);
    }

    std::optional<Status> statusFromName(
        const std::string_view name) noexcept
    {
        return kStatusNames.from(name);
    }

    nlohmann::json puzzleStateToJson(const PuzzleState &state)
    {
        nlohmann::json encoded;

        encoded["givens"] = state.clues().format();
        encoded["cells"] = state.board().format();

        if (state.selected().has_value())
        {
            encoded["chosen"]["row"] = state.selected()->row;
            encoded["chosen"]["col"] = state.selected()->col;
        }

        encoded["note"] = std::string(statusName(state.status()));

        return encoded;

    } // GCOVR_EXCL_LINE

    void puzzleStateFromJson(
        const nlohmann::json &state, PuzzleState &into)
    {
        try
        {
            antwika::replay::validatorFor<stateSchema>().validate(state);
        }
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
        catch (const BoardFormatError &failed) // GCOVR_EXCL_LINE
        {
            throw SnapshotError(
                std::string(
                    "antwika::sudoku: dump holds a board that does "
                    "not parse: ")
                + failed.what());
        }

        const auto note = antwika::enums::fromName<SnapshotError>(
            kStatusNames,
            state.at("note").get<std::string>(),
            "antwika::sudoku: dump names a status this build does not "
            "know: ");

        std::optional<Square> chosen;

        if (state.contains("chosen"))
        {
            chosen = Square{
                .row = state.at("chosen").at("row")
                           .get<std::size_t>(),
                .col = state.at("chosen").at("col")
                           .get<std::size_t>()};
        }

        into.restore(givens, cells, chosen, note);
    }

    SudokuSnapshotStore::SudokuSnapshotStore(
        PuzzleState &state) noexcept
        : antwika::console::IJsonSnapshotStore<
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

}
