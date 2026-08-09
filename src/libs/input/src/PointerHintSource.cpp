#include "antwika/input/PointerHintSource.hpp"

#include <optional>
#include <variant>

#include <antwika/event/ITickEventSource.hpp>

#include "antwika/input/InputEvent.hpp"
#include "antwika/input/Position.hpp"

namespace antwika::input
{

    using antwika::event::ITickEventSource;

    namespace
    {
        [[nodiscard]] std::optional<Position> positionOf(
            const InputEvent &event) noexcept
        {
            if (const auto *moved = std::get_if<PointerMoved>(&event))
            {
                return moved->position;
            }

            if (const auto *pressed =
                    std::get_if<PointerButtonPressed>(&event))
            {
                return pressed->position;
            }

            if (const auto *released =
                    std::get_if<PointerButtonReleased>(&event))
            {
                return released->position;
            }

            return std::nullopt;
        }
    }

    PointerHintSource::PointerHintSource(
        ITickEventSource &inner,
        const IInputEventCodec &codec,
        PointerHintChannel &channel)
        : inner(inner), codec(codec), channel(channel)
    {
    }

    std::vector<Event> PointerHintSource::eventsFor(antwika::time::Tick tick)
    {
        auto events = inner.eventsFor(tick);

        std::optional<Position> at;
        for (const auto &event : events)
        {
            const auto decoded = codec.decode(event);

            if (decoded.has_value())
            {
                if (const auto carried = positionOf(*decoded))
                {
                    at = carried;
                }
            }
        }

        if (at.has_value())
        {
            channel.publish(PointerHint{.position = *at});
        }

        return events;
    } // GCOVR_EXCL_LINE

}
