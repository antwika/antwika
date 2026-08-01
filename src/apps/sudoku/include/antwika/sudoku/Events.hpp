#pragma once

/**
 * @file
 * @brief Names of events defined by this application.
 *
 * Three, and each one carries something that came from outside the
 * program: the puzzle a file or the demo constant supplied, a digit a
 * script asked for, and a request to finish the grid.
 * **None of them carries anything this application can work out
 * again.** The digit a click and a keystroke put in a square, which
 * squares a solve filled, and whether the grid is finished are all
 * regenerated from what is here, which is why there is no
 * `sudoku.cell_filled` and no `sudoku.solved`.
 *
 * `sudoku.solve` is the one worth being explicit about: it records that
 * somebody asked for a solve, never the solution that came back.
 * Storing the solution would store something the solver produces
 * deterministically from a grid a replay already has, and a replay that
 * applied a stored solution would stop being a re-run of the session.
 */
namespace antwika::sudoku::events
{

    /**
     * @brief Starts a session on a puzzle.
     *
     * The payload is a JSON object with one `"cells"` member holding the
     * 81-character flat form Board::parse() reads.
     * It is here rather than a constructor argument because the puzzle
     * arrives from outside the program -- a `--puzzle` file, or the
     * demo constant -- and a recording has to carry it, or replaying one
     * would need the same flag typed again to mean the same thing.
     */
    inline constexpr const char *kNewPuzzle = "sudoku.new_puzzle";

    /**
     * @brief Writes one digit into one square, or empties it.
     *
     * The payload is a JSON object with unsigned integer `"x"` and
     * `"y"` fields naming the square and a `"digit"` field holding 0
     * for a blank or 1-9.
     * A clue is refused, exactly as a keystroke on one is.
     *
     * This is the scripted counterpart of typing, in the same sense
     * `life.toggle_cell` is the scripted counterpart of dragging: a
     * demo replay is written in terms of it, and a live session
     * produces the keystroke instead.
     */
    inline constexpr const char *kSetCell = "sudoku.set_cell";

    /**
     * @brief Asks for the rest of the grid to be filled in.
     *
     * No payload: what a solve produces is a function of the grid it
     * was asked about, so there is nothing here that a replay could not
     * work out for itself.
     */
    inline constexpr const char *kSolve = "sudoku.solve";

} // namespace antwika::sudoku::events
