#include "antwika/game/BindingSource.hpp"

#include <utility>

#include <antwika/event/ITickEventSource.hpp>

#include "antwika/game/Action.hpp"
#include "antwika/game/BindingEvent.hpp"
#include "antwika/game/Events.hpp"

namespace antwika::game
{

    using antwika::event::ITickEventSource;

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

        announced.reset();

        std::vector<Event> opening;

        for (const auto action : kActions)
        {
            const auto key = layout.keyFor(action);

            if (key == kDefaultBindings.keyFor(action))
            {
                continue;
            }

            // GCOVR_EXCL_START
            opening.push_back(
                Event{
                    .name = events::kBindKey,
                    .payload = bindKeyPayload(
                        KeyBinding{.action = action, .key = key})});
            // GCOVR_EXCL_STOP
        }

        opening.insert(
            opening.end(),
            std::make_move_iterator(events.begin()),
            std::make_move_iterator(events.end()));

        return opening;
    }

}
