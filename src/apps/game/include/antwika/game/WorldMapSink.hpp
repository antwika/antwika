#pragma once

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/Key.hpp>

#include "antwika/game/AppMode.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/InputFold.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/WorldMapState.hpp"

namespace antwika::game
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::gfx::Size;

    /**
     * @brief The key that puts a city's map away and shows the world
     * again.
     *
     * Deliberately not Escape, which this app already spends on
     * quitting.
     */
    inline constexpr antwika::input::Key kWorldMapKey =
        antwika::input::Key::M;

    /**
     * @brief Turns this tick's input into which map is showing.
     *
     * **This app defines no event for selecting a city, on purpose.**
     * A click is the input; this sink resolves it against the world
     * map inside the tick path, downstream of the recorder, and
     * switches maps. A replay therefore stores the click and
     * regenerates the switch. An event for "selected a city" would be
     * written alongside the click that caused it, and a replay would
     * then switch twice for one click -- the same trap GridSink
     * documents for placing a tile, and the reason no ui.* event name
     * may ever exist here.
     *
     * | Gesture | Effect |
     * | --- | --- |
     * | left press on a city, world map showing | open that city |
     * | kWorldMapKey, a city showing | go back to the world map |
     *
     * The screen it changes to is *staged* on the AppModeState rather
     * than applied here, so the click that opens a city is not also read
     * as a click on the grid it reveals -- see AppMode.hpp. What is
     * applied at once is the grid swap, which nothing else this tick
     * reads: every collaborator that touches the grid is gated on the
     * mode, and the mode is still the one this tick began in.
     *
     * It gates itself on the mode rather than being wrapped in a
     * ModeGatedSink, because it is the one sink that acts in two modes:
     * a press means a city on the world map and nothing at all on a
     * city's grid, and the key means the reverse.
     *
     * Register it after InputFold, whose current() it reads, and
     * before GridSink, so that a press which selects a city is not
     * also a press on the city's grid.
     *
     * The canvas it resolves a click against must be the size the
     * window was *asked* for, never the size a window reports, for the
     * reason UiOverlay gives: which city a recorded click selects is a
     * function of the layout, and the layout is a function of the
     * canvas.
     */
    class WorldMapSink final : public ITickEventSink
    {
    public:
        /**
         * @brief Construct the sink over what it switches.
         * @param state Which city is open, and every city's grid.
         * @param mode The app's mode; asked for the screen the switch
         * leads to. Must outlive this sink.
         * @param paths The live path index, swapped between cities.
         * @param camera The live camera, swapped between cities.
         * @param input The folded input, holding the event being
         * handled; must be registered ahead of this sink.
         * @param canvas The area the world map is laid out in.
         */
        WorldMapSink(
            WorldMapState &state,
            AppModeState &mode,
            PathIndex &paths,
            Camera &camera,
            const InputFold &input,
            Size canvas);

        WorldMapSink(const WorldMapSink &) = delete;
        WorldMapSink(WorldMapSink &&) = delete;

        WorldMapSink &operator=(const WorldMapSink &) = delete;
        WorldMapSink &operator=(WorldMapSink &&) = delete;

        /**
         * @brief Apply a tick event.
         * @param event An input.* event is acted on; anything else,
         * engine.tick included, is ignored, since which map is showing
         * changes only when somebody says so.
         */
        void handle(const TickEvent &event) override;

    private:
        void openCityUnder(Point pixel);

        WorldMapState &state;
        AppModeState &mode;
        PathIndex &paths;
        Camera &camera;
        const InputFold &input;
        Size canvas;
    };

} // namespace antwika::game
