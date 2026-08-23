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
        [[nodiscard]] std::optional<InputEvent> getMapped(
            const InputEvent &event, const IPointerMapping &mapping)
        {
            if (const auto *moved = std::get_if<PointerMoved>(&event))
            {
                auto rewritten = *moved;
                rewritten.position = mapping.toCanvas(moved->position);

                return InputEvent{rewritten};
            }

            if (const auto *pressedEvent =
                    std::get_if<PointerButtonPressed>(&event))
            {
                auto rewritten = *pressedEvent;
                rewritten.position = mapping.toCanvas(pressedEvent->position);

                return InputEvent{rewritten};
            }

            if (const auto *released =
                    std::get_if<PointerButtonReleased>(&event))
            {
                auto rewritten = *released;
                rewritten.position = mapping.toCanvas(released->position);

                return InputEvent{rewritten};
            }

            return std::nullopt;
        }
    }

    MappedPointerSource::MappedPointerSource(
        ITickEventSource &innerSource,
        const IInputEventCodec &codec,
        const IPointerMapping &mapping)
        : inner(innerSource), codec(codec), mapping(mapping)
    {
    }

    std::vector<Event> MappedPointerSource::eventsFor(
        antwika::time::Tick tick)
    {
        auto events = inner.eventsFor(tick);

        for (auto &event : events)
        {
            const auto decodedEvent = codec.getDecode(event);

            if (!decodedEvent.has_value())
            {
                continue;
            }

            auto rewritten = getMapped(*decodedEvent, mapping);

            if (!rewritten.has_value())
            {
                continue;
            }

            event = codec.getEncode(*rewritten);
        }

        return events;
    }

}
