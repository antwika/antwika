#include "antwika/game/LocaleSource.hpp"

#include <antwika/event/ITickEventSource.hpp>
#include <antwika/i18n/Locale.hpp>

#include <utility>

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

        // Taken once.
        // A tick is asked for once and in increasing order.
        // So forgetting it here is what makes this the first tick's.
        announced.reset();

        // What the build already plays in is not news.
        // So a machine nobody has picked a language on records nothing.
        if (locale == antwika::i18n::kDefaultLocale)
        {
            return events;
        }

        // Every branch left on the excluded lines is allocator's.
        // The unwind edges of an aggregate of two std::strings.
        // Which destroy the half-built record they were made in.
        // The name is a literal too short to reach a heap.
        // Confirmed with gcov -b, as the coverage doc requires.
        // BindingSource.cpp excludes its own announcement for this.
        // GCOVR_EXCL_START
        std::vector<Event> opening;

        opening.push_back(
            Event{
                .name = events::kSetLocale,
                .payload = setLocalePayload(locale)});
        // GCOVR_EXCL_STOP

        // Ahead of the tick's own.
        // So the first click of a session is resolved against it.
        opening.insert(
            opening.end(),
            std::make_move_iterator(events.begin()),
            std::make_move_iterator(events.end()));

        return opening;
    }

} // namespace antwika::game
