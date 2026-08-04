#include "antwika/game/LocaleState.hpp"

#include <antwika/engine/Events.hpp>

#include "antwika/game/Events.hpp"
#include "antwika/game/LocaleEvent.hpp"

namespace antwika::game
{

    void LocaleState::handle(const TickEvent &event)
    {
        if (event.event.name == events::kSetLocale)
        {
            staged = localeFromPayload(event.event.payload);

            return;
        }

        if (event.event.name == antwika::engine::events::kTick)
        {
            // Both, together, and never one without the other.
            // A caption and its language's name cannot then disagree.
            active.setLocale(staged);
            languageNames.setLocale(staged);
        }
    }

} // namespace antwika::game
