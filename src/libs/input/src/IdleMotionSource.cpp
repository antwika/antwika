#include "antwika/input/IdleMotionSource.hpp"

#include <utility>
#include <variant>

#include <antwika/event/ITickEventSource.hpp>

#include "antwika/input/InputEvent.hpp"

namespace antwika::input
{

    using antwika::event::ITickEventSource;

    IdleMotionSource::IdleMotionSource(
        ITickEventSource &inner, const IInputEventCodec &codec)
        : inner(inner), codec(codec)
    {
    }

    std::vector<Event> IdleMotionSource::eventsFor(antwika::time::Tick tick)
    {
        auto events = inner.eventsFor(tick);

        state.beginTick();

        std::vector<Event> kept;
        kept.reserve(events.size() + 1);

        for (auto &event : events)
        {
            const auto decoded = codec.decode(event);

            if (decoded.has_value())
            {
                const auto *moved = std::get_if<PointerMoved>(&*decoded);
                const auto idle = !state.mouse().anyDown();

                state.apply(*decoded);

                if (moved != nullptr && idle)
                {
                    latched = std::move(event);
                    continue;
                }
            }

            if (latched.has_value())
            {
                kept.push_back(std::move(*latched));
                latched.reset();
            }

            kept.push_back(std::move(event));
        }

        return kept;
    }

}
