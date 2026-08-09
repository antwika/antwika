#include "antwika/console/InputFold.hpp"

#include "antwika/console/PointerReading.hpp"

namespace antwika::console
{

    InputFold::InputFold(const IInputEventCodec &codec) : codec(codec)
    {
    }

    void InputFold::handle(const TickEvent &event)
    {
        if (foldedTick != event.tick)
        {
            folded.beginTick();
            foldedTick = event.tick;
        }

        latest = codec.decode(event.event);
        if (!latest.has_value())
        {
            return;
        }

        previous = pointer();
        hasPosition = hasPosition || locates(*latest);
        folded.apply(*latest);
    }

    const std::optional<InputEvent> &InputFold::current() const noexcept
    {
        return latest;
    }

    const InputState &InputFold::state() const noexcept
    {
        return folded;
    }

    Point InputFold::pointer() const noexcept
    {
        return asPoint(folded.mouse().position());
    }

    Point InputFold::pointerBefore() const noexcept
    {
        return previous;
    }

    bool InputFold::located() const noexcept
    {
        return hasPosition;
    }

}
