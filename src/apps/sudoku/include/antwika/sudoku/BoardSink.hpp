#pragma once

#include <cstdint>

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>

#include "antwika/sudoku/PuzzleState.hpp"

namespace antwika::sudoku
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;

    /**
     * @brief Folds this application's own events into the session.
     *
     * A sink beside PlaySink rather than a change to it: this one folds
     * the `sudoku.*` events a demo replay or a test is written in, and
     * PlaySink folds antwika::input's, without either having to know
     * the other exists -- which is the split life::BoardSink and
     * life::PointerToggleSink already have.
     *
     * Everything it does is a function of the event and the state,
     * which is why a live session and its replay reach the same grid:
     * a solve is asked for here and worked out here, never carried in
     * an event.
     */
    class BoardSink final : public ITickEventSink
    {
    public:
        /**
         * @brief Construct the sink over the state it drives.
         * @param state The puzzle, the selection and the last thing
         * said. Must outlive this sink.
         */
        /**
         * @brief Construct the sink over the session it drives.
         * @param state The session; must outlive this sink.
         * @param solveStepBudget How much search one press of Solve
         * may spend.
         */
        BoardSink(PuzzleState &state, std::uint64_t solveStepBudget);

        BoardSink(const BoardSink &) = delete;
        BoardSink(BoardSink &&) = delete;

        BoardSink &operator=(const BoardSink &) = delete;
        BoardSink &operator=(BoardSink &&) = delete;

        /**
         * @brief Apply a tick event's effect to the referenced state.
         * @param event sudoku.new_puzzle starts a session on a grid,
         * sudoku.set_cell writes one digit, sudoku.solve finishes the
         * grid; everything else, engine.tick included, is ignored.
         * @throws BoardFormatError If a payload is not the shape its
         * schema describes, or holds a grid that is not one.
         */
        void handle(const TickEvent &event) override;

    private:
        PuzzleState &state;
        std::uint64_t solveStepBudget;
    };

} // namespace antwika::sudoku
