#pragma once

#include <chrono>

#include <antwika/console/ConsolePicture.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/time/ISleeper.hpp>

#include "antwika/tower_defence/BattleScene.hpp"
#include "antwika/tower_defence/Campaign.hpp"
#include "antwika/tower_defence/ScoreOverlay.hpp"

namespace antwika::tower_defence
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::gfx::IWindow;
    using antwika::gfx::Size;
    using antwika::time::ISleeper;

    class RenderSink final : public ITickEventSink
    {
    public:
        RenderSink(
            IWindow &window,
            const BattleScene &scene,
            const Campaign &campaign,
            const ScoreOverlay &overlay,
            const antwika::console::ConsolePicture &consoleOverlay,
            ISleeper &sleeper,
            std::chrono::milliseconds framePeriod,
            Size canvas);

        RenderSink(const RenderSink &) = delete;
        RenderSink(RenderSink &&) = delete;

        RenderSink &operator=(const RenderSink &) = delete;
        RenderSink &operator=(RenderSink &&) = delete;

        void handle(const TickEvent &event) override;

    private:
        IWindow &window;
        const BattleScene &scene;
        const Campaign &campaign;
        const ScoreOverlay &overlay;
        const antwika::console::ConsolePicture &consoleOverlay;
        ISleeper &sleeper;
        std::chrono::milliseconds framePeriod;
        Size canvas;
    };

}
