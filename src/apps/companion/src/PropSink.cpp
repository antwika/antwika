#include "antwika/companion/PropSink.hpp"

#include <array>
#include <cstddef>
#include <optional>
#include <variant>

#include <antwika/input/InputEvent.hpp>
#include <antwika/input/MouseButton.hpp>

#include "antwika/companion/PetLayout.hpp"

namespace antwika::companion
{

    namespace
    {
        using Verb = void (Pet::*)();

        constexpr std::array<Verb, 3> kVerbs{
            &Pet::feed, &Pet::play, &Pet::putToBed};
    }

    using antwika::input::MouseButton;
    using antwika::input::PointerButtonPressed;

    PropSink::PropSink(
        Pet &pet, const IInputEventCodec &codec, const Size canvas)
        : pet(pet), codec(codec), canvas(canvas)
    {
    }

    void PropSink::handle(const TickEvent &event)
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

        const auto prop = propAt(
            canvas,
            Point{.x = pressed->position.x, .y = pressed->position.y});

        if (!prop)
        {
            pet.pester();
            return;
        }

        (pet.*kVerbs[static_cast<std::size_t>(*prop)])();
    }

}
