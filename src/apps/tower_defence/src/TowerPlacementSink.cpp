#include "antwika/tower_defence/TowerPlacementSink.hpp"

#include <variant>

#include <antwika/input/InputEvent.hpp>
#include <antwika/input/MouseButton.hpp>

namespace antwika::tower_defence
{

    using antwika::input::MouseButton;
    using antwika::input::PointerButtonPressed;

    // The layout is worked out once, here, rather than per event.
    // Neither the canvas nor the level's size can change afterwards.
    TowerPlacementSink::TowerPlacementSink(
        Battle &battle, const IInputEventCodec &codec, const Size canvas)
        : battle(battle),
          codec(codec),
          layout(layoutFor(
              canvas, battle.level().width, battle.level().height))
    {
    }

    void TowerPlacementSink::handle(const TickEvent &event)
    {
        if (!layout)
        {
            return;
        }

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

        const auto cell =
            cellAt(*layout, pressed->position.x, pressed->position.y);
        if (!cell)
        {
            return;
        }

        // A refusal is not an error, so nothing is done with it.
        static_cast<void>(battle.placeTower(*cell));
    }

} // namespace antwika::tower_defence
