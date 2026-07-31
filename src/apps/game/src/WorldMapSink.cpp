#include "antwika/game/WorldMapSink.hpp"

#include <cstddef>
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
        WorldMapState &state,
        AppModeState &mode,
        PathIndex &paths,
        Camera &camera,
        const InputFold &input,
        Size canvas)
        : state(state),
          mode(mode),
          paths(paths),
          camera(camera),
          input(input),
          canvas(canvas)
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
            if (key->key == kWorldMapKey && !key->repeat
                && mode.mode() == AppMode::CityMap)
            {
                state.closeCity(paths, camera);
                mode.request(AppMode::WorldMap);
            }
            return;
        }

        const auto *pressed = std::get_if<PointerButtonPressed>(&*decoded);
        if (pressed == nullptr || pressed->button != MouseButton::Left
            || mode.mode() != AppMode::WorldMap)
        {
            return;
        }

        openCityUnder(Point{pressed->position.x, pressed->position.y});
    }

    void WorldMapSink::openCityUnder(Point pixel)
    {
        const WorldMap &world = state.world();
        const std::optional<Cell> cell =
            worldCellAt(canvas, world.width, world.height, pixel);
        if (!cell.has_value())
        {
            return;
        }

        const std::size_t city = world.cityAt(*cell);
        if (city >= kCityCount)
        {
            return;
        }

        state.openCityAt(city, paths, camera);

        // Staged, not applied.
        // So this very click cannot also be read by the grid it opens.
        mode.request(AppMode::CityMap);
    }

} // namespace antwika::game
