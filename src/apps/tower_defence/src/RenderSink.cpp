#include "antwika/tower_defence/RenderSink.hpp"

#include <antwika/engine/Events.hpp>
#include <antwika/ui/Painter.hpp>

#include "antwika/tower_defence/BattleSnapshot.hpp"

namespace antwika::tower_defence
{

    RenderSink::RenderSink(
        IWindow &window,
        const BattleScene &scene,
        const Campaign &campaign,
        const ScoreOverlay &overlay,
        const antwika::console::ConsolePicture &consoleOverlay,
        ISleeper &sleeper,
        const std::chrono::milliseconds framePeriod,
        const Size canvas)
        : window(window),
          scene(scene),
          campaign(campaign),
          overlay(overlay),
          consoleOverlay(consoleOverlay),
          sleeper(sleeper),
          framePeriod(framePeriod),
          canvas(canvas)
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
        scene.draw(renderer, canvas, snapshotOf(campaign));

        // The bar goes on over the battle, so it reads as in front.
        antwika::ui::paint(renderer, overlay.commands());

        // And the console goes on over everything, sheet on top.
        antwika::ui::paint(renderer, consoleOverlay.commands());
        renderer.present();

        sleeper.sleep(framePeriod);
    }

} // namespace antwika::tower_defence
