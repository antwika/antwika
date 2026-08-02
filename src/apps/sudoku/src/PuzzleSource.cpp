#include "antwika/sudoku/PuzzleSource.hpp"

#include <utility>

#include <nlohmann/json.hpp>

#include "antwika/sudoku/Events.hpp"

namespace antwika::sudoku
{

    Event newPuzzleEvent(const Board &board)
    {
        nlohmann::json payload;
        payload["cells"] = board.format();

        // The excluded lines below are the allocator's alone.
        // Building an Event that owns two strings is what makes them.
        // And gcov puts the cleanup block on the closing brace.
        // See docs/confirming-unreachable-branches.md.
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
            // Every branch on the excluded lines is the allocator's.
            // Those are insert's throw edge and its growth path.
            auto announcement = newPuzzleEvent(*puzzle);
            // GCOVR_EXCL_START
            events.insert(
                events.begin(), std::move(announcement));
            // GCOVR_EXCL_STOP
            puzzle.reset();
        }

        return events;
    } // GCOVR_EXCL_LINE

} // namespace antwika::sudoku
