#pragma once

#include <nlohmann/json_fwd.hpp>

#include <cstdint>
#include <istream>
#include <optional>
#include <string>
#include <string_view>

#include <antwika/replay/MigrationChain.hpp>

#include "antwika/sudoku/Board.hpp"

namespace antwika::sudoku
{

    using antwika::replay::MigrationChain;

    inline constexpr std::string_view kDemoPuzzle =
        "53..7...."
        "6..195..."
        ".98....6."
        "8...6...3"
        "4..8.3..1"
        "7...2...6"
        ".6....28."
        "...419..5"
        "....8..79";

    inline constexpr std::string_view kPuzzleMagic = "antwika-sudoku";

    inline constexpr std::uint32_t kPuzzleDocumentVersion = 2;

    [[nodiscard]] MigrationChain standardPuzzleMigrations();

    [[nodiscard]] nlohmann::json parsePuzzleDocument(
        std::string_view text);

    [[nodiscard]] Board puzzleFromJson(const nlohmann::json &document);

    [[nodiscard]] Board readPuzzle(std::istream &in);

    [[nodiscard]] std::optional<Board> startingPuzzle(
        const std::optional<std::string> &puzzlePath, bool replaying);

}
