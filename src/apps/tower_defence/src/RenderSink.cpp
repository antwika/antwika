#include "antwika/tower_defence/RenderSink.hpp"

#include <antwika/engine/Events.hpp>
#include <antwika/ui/Painter.hpp>

#include "antwika/tower_defence/BattleSnapshot.hpp"

namespace antwika::tower_defence
{

    RenderSink::RenderSink(
        IWindow &window,
        const BattleScene &scene,
        const Battle &battle,
        const ScoreOverlay &overlay,
        ISleeper &sleeper,
        const std::chrono::milliseconds framePeriod,
        const Size canvas)
        : window(window),
          scene(scene),
          battle(battle),
          overlay(overlay),
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
        scene.draw(renderer, canvas, snapshotOf(battle));

        // The bar goes on last, so it reads as being in front.
        antwika::ui::paint(renderer, overlay.commands());
        renderer.present();

        sleeper.sleep(framePeriod);
    }

} // namespace antwika::tower_defence
