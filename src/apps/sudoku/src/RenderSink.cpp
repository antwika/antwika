#include "antwika/sudoku/RenderSink.hpp"

#include <antwika/engine/Events.hpp>
#include <antwika/ui/Painter.hpp>

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

        // The console last, so the sheet stands over the whole board.
        // An empty list while no console is mounted paints nothing.
        antwika::ui::paint(renderer, console.commands());
        renderer.present();

        sleeper.sleep(framePeriod);
    }

} // namespace antwika::sudoku
