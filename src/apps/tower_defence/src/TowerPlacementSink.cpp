#include "antwika/tower_defence/TowerPlacementSink.hpp"

#include <variant>

#include <antwika/input/InputEvent.hpp>
#include <antwika/input/MouseButton.hpp>

#include "antwika/tower_defence/GridLayout.hpp"

namespace antwika::tower_defence
{

    using antwika::input::MouseButton;
    using antwika::input::PointerButtonPressed;

    TowerPlacementSink::TowerPlacementSink(
        Campaign &campaign,
        const IInputEventCodec &codec,
        const Size canvas)
        : campaign(campaign), codec(codec), canvas(canvas)
    {
    }

    void TowerPlacementSink::handle(const TickEvent &event)
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

        const Level &level = campaign.battle().level();
        const auto layout = layoutFor(canvas, level.width, level.height);
        if (!layout)
        {
            return;
        }

        const auto cell =
            cellAt(*layout, pressed->position.x, pressed->position.y);
        if (!cell)
        {
            return;
        }

        static_cast<void>(campaign.placeTower(*cell));
    }

}
