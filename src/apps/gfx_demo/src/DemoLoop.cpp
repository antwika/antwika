#include "antwika/gfx_demo/DemoLoop.hpp"

#include <optional>

#include <antwika/app/PointerReading.hpp>
#include <antwika/app/WindowEvents.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/WindowId.hpp>
#include <antwika/input/InputState.hpp>
#include <antwika/input/PointerHint.hpp>
#include <antwika/input/PointerHintChannel.hpp>
#include <antwika/ui/Hover.hpp>
#include <antwika/ui/Pointer.hpp>
#include <antwika/ui/WidgetId.hpp>

namespace antwika::gfx_demo
{

    using antwika::app::hoverFrom;
    using antwika::app::locates;
    using antwika::app::pointerFrom;
    using antwika::input::InputState;
    using antwika::input::PointerHint;
    using antwika::input::PointerHintChannel;
    using antwika::ui::applyHover;
    using antwika::ui::kNoWidget;
    using antwika::ui::Pointer;

    DemoLoop::DemoLoop(
        IGfxBackend &backend,
        IInputBackend &input,
        const DemoScene &scene,
        ISleeper &sleeper,
        std::chrono::milliseconds framePeriod)
        : backend(backend),
          input(input),
          scene(scene),
          sleeper(sleeper),
          framePeriod(framePeriod)
    {
    }

    void DemoLoop::run(
        const WindowDesc &desc,
        const Bitmap &logo,
        std::optional<std::uint32_t> maxFrames)
    {
        const auto window = backend.createWindow(desc);

        const auto texture = window->renderer().createTexture(logo);

        InputState state;
        bool located = false;

        PointerHintChannel hints;

        for (std::uint32_t frame = 0;
             !maxFrames.has_value() || frame < maxFrames.value();
             ++frame)
        {
            if (antwika::app::closeRequestedOn(backend, window->id()))
            {
                window->close();
            }

            if (!window->isOpen())
            {
                break;
            }

            state.beginTick();

            while (const auto event = input.pollEvent())
            {
                located = located || locates(*event);
                state.apply(*event);
            }

            if (located)
            {
                hints.publish(
                    PointerHint{.position = state.mouse().position()});
            }

            const auto folded = pointerFrom(state, located);

            const Pointer pointer{
                .position = folded.down ? folded.position : std::nullopt,
                .down = folded.down,
                .pressed = folded.pressed};

            const auto canvas = window->size();
            auto picture = scene.describe(canvas, pointer, clickCount);
            const auto activated = picture.interactions.activated;

            if (activated == widgets::kCount)
            {
                ++clickCount;
            }
            else if (activated == widgets::kReset)
            {
                clickCount = 0;
            }

            if (activated != kNoWidget)
            {
                picture = scene.describe(canvas, pointer, clickCount);
            }

            applyHover(
                picture.commands,
                picture.hoverTargets,
                hoverFrom(hints.forRenderingOnly()));

            scene.draw(
                window->renderer(), canvas, *texture, picture.commands);
            window->renderer().present();

            sleeper.sleep(framePeriod);
        }

        window->close();
    }

    std::uint32_t DemoLoop::clicks() const noexcept
    {
        return clickCount;
    }

}
