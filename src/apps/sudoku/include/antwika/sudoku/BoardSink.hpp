#pragma once

#include <cstdint>

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>

#include "antwika/sudoku/PuzzleState.hpp"

namespace antwika::sudoku
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;

    class BoardSink final : public ITickEventSink
    {
    public:
        BoardSink(PuzzleState &state, std::uint64_t solveStepBudget);

        BoardSink(const BoardSink &) = delete;
        BoardSink(BoardSink &&) = delete;

        BoardSink &operator=(const BoardSink &) = delete;
        BoardSink &operator=(BoardSink &&) = delete;

        void handle(const TickEvent &event) override;

    private:
        PuzzleState &state;
        std::uint64_t solveStepBudget;
    };

}
