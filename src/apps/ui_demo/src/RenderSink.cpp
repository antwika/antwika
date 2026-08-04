#include "antwika/ui_demo/RenderSink.hpp"

#include <antwika/app/FramePresentation.hpp>

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
        if (!antwika::app::drawsOn(event, window))
        {
            return;
        }

        // No console is mounted here, so the scene is the frame.
        antwika::app::presentFrame(
            window,
            [this](antwika::gfx::IRenderer &renderer)
            {
                scene.draw(renderer, overlay.commands());
            });

        sleeper.sleep(framePeriod);
    }

} // namespace antwika::ui_demo
