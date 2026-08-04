#include "antwika/tower_defence/RenderSink.hpp"

#include <antwika/app/FramePresentation.hpp>

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
        if (!antwika::app::drawsOn(event, window))
        {
            return;
        }

        // The console goes on over everything, sheet on top.
        antwika::app::presentFrame(
            window,
            consoleOverlay,
            [this](antwika::gfx::IRenderer &renderer)
            {
                scene.draw(renderer, canvas, snapshotOf(campaign));

                // The bar goes over the battle, so it reads in front.
                antwika::app::paintOver(renderer, overlay);
            });

        sleeper.sleep(framePeriod);
    }

} // namespace antwika::tower_defence
