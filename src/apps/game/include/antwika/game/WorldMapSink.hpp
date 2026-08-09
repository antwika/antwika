#pragma once

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/Key.hpp>

#include "antwika/game/AppMode.hpp"
#include "antwika/game/InputFold.hpp"
#include "antwika/game/LiveGrid.hpp"
#include "antwika/game/WorldMapState.hpp"

namespace antwika::game
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::gfx::Size;

    inline constexpr antwika::input::Key kWorldMapKey =
        antwika::input::Key::M;

    class WorldMapSink final : public ITickEventSink
    {
    public:
        WorldMapSink(
            WorldMapState &state,
            AppModeState &mode,
            const LiveGrid &live,
            const InputFold &input,
            Size canvas);

        WorldMapSink(const WorldMapSink &) = delete;
        WorldMapSink(WorldMapSink &&) = delete;

        WorldMapSink &operator=(const WorldMapSink &) = delete;
        WorldMapSink &operator=(WorldMapSink &&) = delete;

        void handle(const TickEvent &event) override;

    private:
        void openCityUnder(Point pixel);

        WorldMapState &state;
        AppModeState &mode;
        LiveGrid live;
        const InputFold &input;
        Size canvas;
    };

}
