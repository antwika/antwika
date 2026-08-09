#pragma once

#include <chrono>

#include <antwika/console/ConsolePicture.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/time/ISleeper.hpp>

#include "antwika/sudoku/BoardOverlay.hpp"
#include "antwika/sudoku/SudokuScene.hpp"

namespace antwika::sudoku
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::gfx::IWindow;
    using antwika::time::ISleeper;

    class RenderSink final : public ITickEventSink
    {
    public:
        RenderSink(
            IWindow &window,
            const SudokuScene &scene,
            const BoardOverlay &overlay,
            const antwika::console::ConsolePicture &console,
            ISleeper &sleeper,
            std::chrono::milliseconds framePeriod);

        RenderSink(const RenderSink &) = delete;
        RenderSink(RenderSink &&) = delete;

        RenderSink &operator=(const RenderSink &) = delete;
        RenderSink &operator=(RenderSink &&) = delete;

        void handle(const TickEvent &event) override;

    private:
        IWindow &window;
        const SudokuScene &scene;
        const BoardOverlay &overlay;
        const antwika::console::ConsolePicture &console;
        ISleeper &sleeper;
        std::chrono::milliseconds framePeriod;
    };

}
