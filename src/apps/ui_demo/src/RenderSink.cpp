#include "antwika/ui_demo/RenderSink.hpp"

#include <antwika/engine/Events.hpp>

namespace antwika::ui_demo
{

    RenderSink::RenderSink(
        IWindow &window,
        const DemoScene &scene,
        const DemoOverlay &overlay,
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

} // namespace antwika::ui_demo
