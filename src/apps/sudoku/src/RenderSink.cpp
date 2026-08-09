#include "antwika/sudoku/RenderSink.hpp"

#include <antwika/app/FramePresentation.hpp>

namespace antwika::sudoku
{

    RenderSink::RenderSink(
        IWindow &window,
        const SudokuScene &scene,
        const BoardOverlay &overlay,
        const antwika::console::ConsolePicture &console,
        ISleeper &sleeper,
        const std::chrono::milliseconds framePeriod)
        : window(window),
          scene(scene),
          overlay(overlay),
          console(console),
          sleeper(sleeper),
          framePeriod(framePeriod)
    {
    }

    void RenderSink::handle(const TickEvent &event)
    {
        if (!antwika::app::drawsOn(event, window))
        {
            return;
        }

        antwika::app::presentFrame(
            window,
            console,
            [this](antwika::gfx::IRenderer &renderer)
            {
                scene.draw(renderer, overlay.commands());
            });

        sleeper.sleep(framePeriod);
    }

}
