#include "antwika/game/LocaleSource.hpp"

#include <utility>

#include <antwika/event/ITickEventSource.hpp>
#include <antwika/i18n/Locale.hpp>

#include "antwika/game/Events.hpp"
#include "antwika/game/LocaleEvent.hpp"

namespace antwika::game
{

    using antwika::event::ITickEventSource;

    LocaleSource::LocaleSource(
        ITickEventSource &inner,
        std::optional<Locale> announced) noexcept
        : inner(inner), announced(announced)
    {
    }

    std::vector<Event> LocaleSource::eventsFor(antwika::time::Tick tick)
    {
        auto events = inner.eventsFor(tick);

        if (!announced.has_value())
        {
            return events;
        }

        const auto locale = *announced;

        announced.reset();

        if (locale == antwika::i18n::kDefaultLocale)
        {
            return events;
        }

        // GCOVR_EXCL_START
        std::vector<Event> opening;

        opening.push_back(
            Event{
                .name = events::kSetLocale,
                .payload = setLocalePayload(locale)});
        // GCOVR_EXCL_STOP

        opening.insert(
            opening.end(),
            std::make_move_iterator(events.begin()),
            std::make_move_iterator(events.end()));

        return opening;
    }

}
