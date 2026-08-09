#include "antwika/sudoku/PuzzleSource.hpp"

#include <nlohmann/json.hpp>

#include <utility>

#include <antwika/event/ITickEventSource.hpp>

#include "antwika/sudoku/Events.hpp"

namespace antwika::sudoku
{

    using antwika::event::ITickEventSource;

    Event newPuzzleEvent(const Board &board)
    {
        nlohmann::json payload;
        payload["cells"] = board.format();

        // GCOVR_EXCL_START
        return Event{
            .name = events::kNewPuzzle, .payload = payload.dump()};
    } // GCOVR_EXCL_STOP

    PuzzleSource::PuzzleSource(
        ITickEventSource &inner, std::optional<Board> puzzle)
        : inner(inner), puzzle(std::move(puzzle))
    {
    }

    std::vector<Event> PuzzleSource::eventsFor(
        const antwika::time::Tick tick)
    {
        auto events = inner.eventsFor(tick);

        if (puzzle.has_value())
        {
            auto announcement = newPuzzleEvent(*puzzle);
            // GCOVR_EXCL_START
            events.insert(
                events.begin(), std::move(announcement));
            // GCOVR_EXCL_STOP
            puzzle.reset();
        }

        return events;
    } // GCOVR_EXCL_LINE

}
