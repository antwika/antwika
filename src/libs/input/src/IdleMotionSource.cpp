#include "antwika/input/IdleMotionSource.hpp"

#include <utility>
#include <variant>

#include "antwika/input/InputEvent.hpp"

namespace antwika::input
{

    IdleMotionSource::IdleMotionSource(
        IReplaySource &inner, const IInputEventCodec &codec)
        : inner(inner), codec(codec)
    {
    }

    std::vector<Event> IdleMotionSource::eventsFor(antwika::time::Tick tick)
    {
        auto events = inner.eventsFor(tick);

        // Only anyDown() is read below, which this does not clear.
        // It is called so the per-tick sums cannot run away unread.
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
                    // Whatever was latched was superseded unread.
                    latched = std::move(event);
                    continue;
                }
            }

            // This is the first moment anything could read a position.
            if (latched.has_value())
            {
                kept.push_back(std::move(*latched));
                latched.reset();
            }

            kept.push_back(std::move(event));
        }

        return kept;
    }

} // namespace antwika::input
