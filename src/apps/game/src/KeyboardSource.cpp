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

        announced.reset();

        if (layout == kDefaultKeyboardLayout)
        {
            return events;
        }

        // GCOVR_EXCL_START
        std::vector<Event> opening;

        opening.push_back(
            Event{
                .name = events::kSetKeyboard,
                .payload = setKeyboardPayload(layout)});
        // GCOVR_EXCL_STOP

        opening.insert(
            opening.end(),
            std::make_move_iterator(events.begin()),
            std::make_move_iterator(events.end()));

        return opening;
    }

}
