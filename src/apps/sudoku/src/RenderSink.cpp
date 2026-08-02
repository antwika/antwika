#include "antwika/sudoku/RenderSink.hpp"

#include <antwika/engine/Events.hpp>

namespace antwika::sudoku
{

    RenderSink::RenderSink(
        IWindow &window,
        const SudokuScene &scene,
        const BoardOverlay &overlay,
        ISleeper &sleeper,
        const std::chrono::milliseconds framePeriod)
        : window(window),
          scene(scene),
          overlay(overlay),
          sleeper(sleeper),
          framePeriod(framePeriod)
    {
    }

    void RenderSink::handle(const TickEvent &event)
    {
        if (event.event.name != antwika::engine::events::kTick)
        {
            return;
        }

        if (!window.isOpen())
        {
            return;
        }

        auto &renderer = window.renderer();
        scene.draw(renderer, overlay.commands());
        renderer.present();

        sleeper.sleep(framePeriod);
    }

} // namespace antwika::sudoku
