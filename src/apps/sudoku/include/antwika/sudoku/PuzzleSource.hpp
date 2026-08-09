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

    [[nodiscard]] Event newPuzzleEvent(const Board &board);

    class PuzzleSource final : public ITickEventSource
    {
    public:
        PuzzleSource(
            ITickEventSource &inner, std::optional<Board> puzzle);

        PuzzleSource(const PuzzleSource &) = delete;
        PuzzleSource(PuzzleSource &&) = delete;

        PuzzleSource &operator=(const PuzzleSource &) = delete;
        PuzzleSource &operator=(PuzzleSource &&) = delete;

        [[nodiscard]] std::vector<Event> eventsFor(
            antwika::time::Tick tick) override;

    private:
        ITickEventSource &inner;
        std::optional<Board> puzzle;
    };

}
