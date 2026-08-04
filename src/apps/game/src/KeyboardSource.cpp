#include "antwika/game/KeyboardSource.hpp"

#include <iterator>

#include "antwika/game/Events.hpp"
#include "antwika/game/KeyboardEvent.hpp"

namespace antwika::game
{

    KeyboardSource::KeyboardSource(
        ITickEventSource &inner,
        std::optional<KeyboardLayout> announced) noexcept
        : inner(inner), announced(announced)
    {
    }

    std::vector<Event> KeyboardSource::eventsFor(
        antwika::time::Tick tick)
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

        // What the build already types by is not news.
        // So a machine nobody has picked a board on records nothing.
        if (layout == kDefaultKeyboardLayout)
        {
            return events;
        }

        // Every branch left on the excluded lines is the allocator's.
        // The unwind edges of an aggregate of two std::strings.
        // LocaleSource.cpp excludes its own announcement for this.
        // GCOVR_EXCL_START
        std::vector<Event> opening;

        opening.push_back(
            Event{
                .name = events::kSetKeyboard,
                .payload = setKeyboardPayload(layout)});
        // GCOVR_EXCL_STOP

        // Ahead of the tick's own.
        // So the first keystroke of a session types by it.
        opening.insert(
            opening.end(),
            std::make_move_iterator(events.begin()),
            std::make_move_iterator(events.end()));

        return opening;
    }

} // namespace antwika::game
