#include "antwika/companion/TapSink.hpp"

#include <variant>

#include <antwika/input/InputEvent.hpp>
#include <antwika/input/MouseButton.hpp>

namespace antwika::companion
{

    using antwika::input::MouseButton;
    using antwika::input::PointerButtonPressed;

    TapSink::TapSink(Pet &pet, const IInputEventCodec &codec)
        : pet(pet), codec(codec)
    {
    }

    void TapSink::handle(const TickEvent &event)
    {
        const auto edge = codec.decode(event.event);

        if (!edge)
        {
            return;
        }

        const auto *pressed = std::get_if<PointerButtonPressed>(&*edge);

        if (pressed == nullptr || pressed->button != MouseButton::Left)
        {
            return;
        }

        pet.tap();
    }

} // namespace antwika::companion
