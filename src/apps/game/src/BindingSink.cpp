#include "antwika/game/BindingSink.hpp"

#include "antwika/game/BindingEvent.hpp"
#include "antwika/game/Events.hpp"
#include "antwika/game/KeyboardEvent.hpp"

namespace antwika::game
{

    BindingSink::BindingSink(OptionsState &options) noexcept
        : options(options)
    {
    }

    void BindingSink::handle(const TickEvent &event)
    {
        if (event.event.name == events::kSetKeyboard)
        {
            options.setKeyboard(
                keyboardFromPayload(event.event.payload));
            return;
        }

        if (event.event.name != events::kBindKey)
        {
            return;
        }

        const auto binding = bindKeyFromPayload(event.event.payload);

        options.apply(binding.action, binding.key);
    }

}
