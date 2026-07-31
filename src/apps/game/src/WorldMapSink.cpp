#include "antwika/game/WorldMapSink.hpp"

#include <optional>
#include <variant>

#include <antwika/input/InputEvent.hpp>
#include <antwika/input/MouseButton.hpp>

#include "antwika/game/Cell.hpp"
#include "antwika/game/WorldMapLayout.hpp"

namespace antwika::game
{

    using antwika::input::InputEvent;
    using antwika::input::KeyPressed;
    using antwika::input::MouseButton;
    using antwika::input::PointerButtonPressed;

    WorldMapSink::WorldMapSink(
        WorldMapState &state, const InputFold &input, Size canvas)
        : state(state), input(input), canvas(canvas)
    {
    }

    void WorldMapSink::handle(const TickEvent &)
    {
        // Whatever the fold was just given, since it runs first.
        const std::optional<InputEvent> &decoded = input.current();
        if (!decoded.has_value())
        {
            return;
        }

        if (const auto *key = std::get_if<KeyPressed>(&*decoded);
            key != nullptr)
        {
            // A repeat is a held key, not a fresh press.
            // Holding it should not keep re-closing the map.
            if (key->key == kWorldMapKey && !key->repeat)
            {
                state.closeCity();
            }
            return;
        }

        const auto *pressed = std::get_if<PointerButtonPressed>(&*decoded);
        if (pressed == nullptr || pressed->button != MouseButton::Left
            || state.view() != MapView::World)
        {
            return;
        }

        const WorldMap &world = state.world();
        const std::optional<Cell> cell = worldCellAt(
            canvas,
            world.width,
            world.height,
            Point{pressed->position.x, pressed->position.y});
        if (!cell.has_value())
        {
            return;
        }

        const std::size_t city = world.cityAt(*cell);
        if (city < kCityCount)
        {
            state.openCityAt(city);
        }
    }

} // namespace antwika::game
