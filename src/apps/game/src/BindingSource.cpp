#include "antwika/game/BindingSource.hpp"

#include <utility>

#include "antwika/game/Action.hpp"
#include "antwika/game/BindingEvent.hpp"
#include "antwika/game/Events.hpp"

namespace antwika::game
{

    BindingSource::BindingSource(
        ITickEventSource &inner,
        std::optional<KeyBindings> announced) noexcept
        : inner(inner), announced(std::move(announced))
    {
    }

    std::vector<Event> BindingSource::eventsFor(antwika::time::Tick tick)
    {
        auto events = inner.eventsFor(tick);

        if (!announced.has_value())
        {
            return events;
        }

        const auto layout = *announced;

        // Taken once.
        // A tick is asked for once and in increasing order.
        // So forgetting it here is what makes this the first tick's.
        announced.reset();

        std::vector<Event> opening;

        for (const auto action : kActions)
        {
            const auto key = layout.keyFor(action);

            // What the build already plays is not news.
            // So a machine nobody has rebound records nothing at all.
            if (key == kDefaultBindings.keyFor(action))
            {
                continue;
            }

            // Every branch left on the excluded lines is allocator's.
            // The unwind edges of an aggregate of two std::strings.
            // Which destroy the half-built record they were made in.
            // The name is a literal too short to reach a heap.
            // Confirmed with gcov -b, as the coverage doc requires.
            // Game.cpp excludes its own GameSummary for this reason.
            // GCOVR_EXCL_START
            opening.push_back(
                Event{
                    .name = events::kBindKey,
                    .payload = bindKeyPayload(
                        KeyBinding{.action = action, .key = key})});
            // GCOVR_EXCL_STOP
        }

        // Ahead of the tick's own.
        // So the first click of a session is resolved against it.
        opening.insert(
            opening.end(),
            std::make_move_iterator(events.begin()),
            std::make_move_iterator(events.end()));

        return opening;
    }

} // namespace antwika::game
