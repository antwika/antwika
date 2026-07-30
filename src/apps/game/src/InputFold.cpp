#include "antwika/game/InputFold.hpp"

#include "antwika/game/PointerReading.hpp"

namespace antwika::game
{

    InputFold::InputFold(const IInputEventCodec &codec) : codec(codec)
    {
    }

    void InputFold::handle(const TickEvent &event)
    {
        // A tick's edges are cleared on the next tick's first event.
        // Clearing at the end of that tick would need this sink last.
        // Nothing can then read an edge its own tick has lost.
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

} // namespace antwika::game
