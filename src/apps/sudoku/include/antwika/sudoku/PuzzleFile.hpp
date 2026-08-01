#pragma once

#include <cstdint>
#include <istream>
#include <optional>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

#include <antwika/replay/MigrationChain.hpp>

#include "antwika/sudoku/Board.hpp"

namespace antwika::sudoku
{

    using antwika::replay::MigrationChain;

    /**
     * @brief A well-known easy puzzle, used when nothing names one.
     */
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

    /**
     * @brief What every version 2 puzzle document says it is.
     *
     * Checked before anything else, so a replay or a companion save
     * handed to `--puzzle` is refused as the wrong kind of file rather
     * than as a puzzle with every member missing: all three state their
     * version in the same member, and the magic is what tells them
     * apart.
     */
    inline constexpr std::string_view kPuzzleMagic = "antwika-sudoku";

    /**
     * @brief Which revision of the puzzle format this build reads.
     *
     * Version 1 is the classic flat 81-character grid the rest of the
     * world writes -- `53..7....` and so on, blanks as `.` or `0`. It
     * predates this mechanism and says nothing about a version, which
     * is exactly the case antwika::replay::kUnversionedDocumentVersion
     * exists for, so it needs no change on anybody's disk to keep
     * loading.
     * Version 2 is that grid inside a JSON object that says which
     * format it is, which is what makes a `--puzzle` file tellable from
     * every other document this project writes.
     */
    inline constexpr std::uint32_t kPuzzleDocumentVersion = 2;

    /**
     * @brief Build the migration chain for the puzzle format.
     *
     * This application's answer to standardReplayMigrations(): its own
     * list, its own current version, and no registry anywhere, so this
     * chain and the replay's cannot see each other.
     *
     * @return The chain, from version 1 to the current one.
     */
    [[nodiscard]] MigrationChain standardPuzzleMigrations();

    /**
     * @brief Parse a puzzle document's text into JSON.
     *
     * The `parse` of `parse -> read version -> migrate -> validate ->
     * decode`, and the only step that has to know there are two shapes
     * of this document at all.
     * A document whose first non-space character is `{` is JSON; any
     * other is the flat version 1 grid, which is wrapped into the one
     * member every later version keeps it in.
     *
     * That is decided on the first character rather than by trying JSON
     * and falling back, because a finished grid is 81 digits and 81
     * digits *are* valid JSON -- a number. Sniffing by whether a parse
     * succeeds would read the one puzzle that needs no solving as a
     * document of the wrong kind.
     *
     * @param text The document as it was read.
     * @return The parsed document, not yet migrated or validated.
     * @throws BoardFormatError If the text opens like JSON and is not.
     */
    [[nodiscard]] nlohmann::json parsePuzzleDocument(
        std::string_view text);

    /**
     * @brief Decode a puzzle from a parsed document.
     *
     * Reads it as `read version -> migrate -> validate -> decode`,
     * migrating before validating so that exactly one schema exists
     * rather than one per revision.
     *
     * @param document The parsed document.
     * @return The board it describes.
     * @throws BoardFormatError If the document is not a puzzle this
     * build can read, or holds 81 characters that are not a grid.
     */
    [[nodiscard]] Board puzzleFromJson(const nlohmann::json &document);

    /**
     * @brief Read a puzzle from a stream.
     *
     * A stream rather than a path, following
     * companion::readCompanionMemory() and antwika::gfx::PngReader:
     * nothing here opens a file, so every refusal this can produce is
     * reachable from bytes in memory.
     *
     * @param in The stream to read.
     * @return The board it describes.
     * @throws BoardFormatError If it is not a puzzle this build can
     * read.
     */
    [[nodiscard]] Board readPuzzle(std::istream &in);

    /**
     * @brief Work out which puzzle a session starts on.
     *
     * Nothing when the session is a replay: the recording carries its
     * own `sudoku.new_puzzle`, so supplying a second one would start
     * the session on a grid the recorded clicks were never aimed at.
     * That is the whole reason the puzzle travels as an event rather
     * than as a constructor argument.
     *
     * @param puzzlePath What `--puzzle` named, if anything.
     * @param replaying Whether a `--replay` file is driving the run.
     * @return The puzzle to announce, or nothing when one is already
     * on its way.
     * @throws BoardFormatError If a named file cannot be opened or is
     * not a puzzle this build can read.
     */
    [[nodiscard]] std::optional<Board> startingPuzzle(
        const std::optional<std::string> &puzzlePath, bool replaying);

} // namespace antwika::sudoku
