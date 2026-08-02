#include "antwika/game/BindingSink.hpp"

#include "antwika/game/BindingEvent.hpp"
#include "antwika/game/Events.hpp"

namespace antwika::game
{

    BindingSink::BindingSink(OptionsState &options) noexcept
        : options(options)
    {
    }

    void BindingSink::handle(const TickEvent &event)
    {
        if (event.event.name != events::kBindKey)
        {
            return;
        }

        const auto binding = bindKeyFromPayload(event.event.payload);

        // Whatever it comes to is the answer.
        // A layout naming one key twice this build cannot write.
        // And the reader that would have refuses one.
        // So what is left here is a hand-edited recording.
        // The last word on a key is what a total function gives it.
        options.apply(binding.action, binding.key);
    }

} // namespace antwika::game
