#pragma once

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/Key.hpp>

#include "antwika/game/InputFold.hpp"
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
         * @param state Which map is showing; written by this sink.
         * @param input The folded input, holding the event being
         * handled; must be registered ahead of this sink.
         * @param canvas The area the world map is laid out in.
         */
        WorldMapSink(
            WorldMapState &state, const InputFold &input, Size canvas);

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
        WorldMapState &state;
        const InputFold &input;
        Size canvas;
    };

} // namespace antwika::game
