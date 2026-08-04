#pragma once

#include <optional>
#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/event/ITickEventSource.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/sudoku/Board.hpp"

namespace antwika::sudoku
{

    using antwika::event::Event;
    using antwika::event::ITickEventSource;

    /**
     * @brief Encode a board as this application's sudoku.new_puzzle
     * event.
     * @param board The puzzle to announce.
     * @return The event, payload and all.
     */
    [[nodiscard]] Event newPuzzleEvent(const Board &board);

    /**
     * @brief Announces the puzzle a live session starts on, once.
     *
     * The puzzle comes from outside the program -- a `--puzzle` file,
     * or the demo constant -- so it is external input, and external
     * input reaches a simulation through the source the loop pulls
     * from. Putting it here rather than into a constructor is what puts
     * it *upstream of the recorder*, so a `--record` run writes the
     * grid it was played on into its own file and replaying that file
     * needs no flag repeated to land the same clicks on the same
     * squares.
     *
     * It is the only thing this application adds to the stream, and it
     * adds nothing at all on a replay: a recording already carries its
     * puzzle, and a second one would overwrite the grid halfway through
     * the session that was recorded on it.
     */
    class PuzzleSource final : public ITickEventSource
    {
    public:
        /**
         * @brief Construct the decorator over what it wraps.
         * @param inner The source whose events pass through; must
         * outlive this object.
         * @param puzzle The grid to announce on the first tick, or
         * nothing when a replay is carrying its own.
         */
        PuzzleSource(
            ITickEventSource &inner, std::optional<Board> puzzle);

        PuzzleSource(const PuzzleSource &) = delete;
        PuzzleSource(PuzzleSource &&) = delete;

        PuzzleSource &operator=(const PuzzleSource &) = delete;
        PuzzleSource &operator=(PuzzleSource &&) = delete;

        /**
         * @brief Get a tick's events, the puzzle ahead of the first
         * tick's.
         *
         * Ahead of them rather than after, so a script may set a square
         * on the very tick the grid arrives on.
         * Announced on the first tick this is asked about rather than
         * on a tick named by number: a source is asked once per tick in
         * increasing order, so the first question is the first tick,
         * and holding the puzzle until it is asked needs no second
         * statement of what the loop counts from.
         *
         * @param tick The tick to fetch events for.
         * @return The wrapped source's events, preceded on the first
         * tick by one sudoku.new_puzzle.
         */
        [[nodiscard]] std::vector<Event> eventsFor(
            antwika::time::Tick tick) override;

    private:
        ITickEventSource &inner;
        std::optional<Board> puzzle;
    };

} // namespace antwika::sudoku
