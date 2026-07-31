#include "antwika/input/PointerHintSource.hpp"

#include <optional>
#include <variant>

#include "antwika/input/InputEvent.hpp"
#include "antwika/input/Position.hpp"

namespace antwika::input
{

    namespace
    {
        /**
         * @brief Get the position an edge carries, if it carries one.
         * @param event The edge to inspect.
         * @return Where the pointer was, or nullopt for an edge that
         * says nothing about where it is -- a wheel notch and a key both
         * happen wherever the pointer already was.
         */
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
    } // namespace

    PointerHintSource::PointerHintSource(
        ITickSource &inner,
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
                // A later position supersedes an earlier one outright.
                // Where the pointer is is not a sum of where it went.
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

        // Untouched, which is the whole guarantee of this class.
        // The closing brace below is where the vector's destructor runs.
        // Only an unwind out of this function ever reaches it.
        return events;
    } // GCOVR_EXCL_LINE

} // namespace antwika::input
