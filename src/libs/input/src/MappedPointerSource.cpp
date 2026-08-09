#include "antwika/input/MappedPointerSource.hpp"

#include <optional>
#include <variant>

#include <antwika/event/ITickEventSource.hpp>

#include "antwika/input/InputEvent.hpp"

namespace antwika::input
{

    using antwika::event::ITickEventSource;

    namespace
    {
        [[nodiscard]] std::optional<InputEvent> mapped(
            const InputEvent &event, const IPointerMapping &mapping)
        {
            if (const auto *moved = std::get_if<PointerMoved>(&event))
            {
                auto rewritten = *moved;
                rewritten.position = mapping.toSurface(moved->position);

                return InputEvent{rewritten};
            }

            if (const auto *pressed =
                    std::get_if<PointerButtonPressed>(&event))
            {
                auto rewritten = *pressed;
                rewritten.position = mapping.toSurface(pressed->position);

                return InputEvent{rewritten};
            }

            if (const auto *released =
                    std::get_if<PointerButtonReleased>(&event))
            {
                auto rewritten = *released;
                rewritten.position = mapping.toSurface(released->position);

                return InputEvent{rewritten};
            }

            return std::nullopt;
        }
    }

    MappedPointerSource::MappedPointerSource(
        ITickEventSource &inner,
        const IInputEventCodec &codec,
        const IPointerMapping &mapping)
        : inner(inner), codec(codec), mapping(mapping)
    {
    }

    std::vector<Event> MappedPointerSource::eventsFor(
        antwika::time::Tick tick)
    {
        auto events = inner.eventsFor(tick);

        for (auto &event : events)
        {
            const auto decoded = codec.decode(event);

            if (!decoded.has_value())
            {
                continue;
            }

            auto rewritten = mapped(*decoded, mapping);

            if (!rewritten.has_value())
            {
                continue;
            }

            event = codec.encode(*rewritten);
        }

        return events;
    }

}
