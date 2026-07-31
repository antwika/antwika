#include "antwika/gfx_demo/DemoLoop.hpp"

#include <optional>
#include <variant>

#include <antwika/app/PointerReading.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/WindowEvent.hpp>
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
    using antwika::gfx::CloseRequested;
    using antwika::input::InputState;
    using antwika::input::PointerHint;
    using antwika::input::PointerHintChannel;
    using antwika::ui::applyHover;
    using antwika::ui::kNoWidget;
    using antwika::ui::Pointer;

    DemoLoop::DemoLoop(
        IGfxBackend &backend, IInputBackend &input, const DemoScene &scene)
        : backend(backend), input(input), scene(scene)
    {
    }

    void DemoLoop::run(
        const WindowDesc &desc,
        const Bitmap &logo,
        std::optional<std::uint32_t> maxFrames)
    {
        const auto window = backend.createWindow(desc);

        // After the window, since a backend may have no device yet.
        // Declared after it too, so it is destroyed first.
        const auto texture = window->renderer().createTexture(logo);

        InputState state;
        bool located = false;

        // Where the pointer is, published for drawing and nothing else.
        // An app with a tick loop has input::PointerHintSource fill it.
        // This demo has no pipeline, so it publishes its own.
        // The channel is a value cell either way.
        PointerHintChannel hints;

        for (std::uint32_t frame = 0;
             !maxFrames.has_value() || frame < maxFrames.value();
             ++frame)
        {
            while (const auto event = backend.pollEvent())
            {
                // The backend pumps one queue for all its windows.
                // An event for somebody else's window is not ours.
                if (event->window != window->id())
                {
                    continue;
                }

                if (std::holds_alternative<CloseRequested>(event->payload))
                {
                    window->close();
                }
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

            // Saying that a folded pointer is the pointer a UI wants.
            // antwika::app is where an application is allowed to say it.
            const auto folded = pointerFrom(state, located);

            // What a recording would carry, modelled by hand.
            // input::IdleMotionSource gates motion between clicks out.
            // So a gated UI is told where the pointer is on a press.
            // This demo has no recorder, so it gates the position here.
            const Pointer pointer{
                .position = folded.down ? folded.position : std::nullopt,
                .down = folded.down,
                .pressed = folded.pressed};

            // The size the window reports, not the size it was asked for.
            // Every other app here refuses to lay a UI out against that.
            // This demo lays out and hit-tests it in the same frame.
            // Nothing records the click, so no later run can disagree.
            // An app with a replay must use the configured size instead.
            // A hit-test follows the layout, and a layout the canvas.
            // See game::UiOverlay, which owns a canvas for that reason.
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

            // Describing again is what shows the new count at once.
            // Otherwise it would appear a frame after the press.
            if (activated != kNoWidget)
            {
                picture = scene.describe(canvas, pointer, clickCount);
            }

            // Last, and only the picture.
            // Every interaction above came off the gated pointer.
            // This repaints what the free-moving one is over.
            // So a button lights up on approach, recorded nowhere.
            applyHover(
                picture.commands,
                picture.hoverTargets,
                hoverFrom(hints.forRenderingOnly()));

            scene.draw(
                window->renderer(), canvas, *texture, picture.commands);
            window->renderer().present();
        }

        window->close();
    }

    std::uint32_t DemoLoop::clicks() const noexcept
    {
        return clickCount;
    }

} // namespace antwika::gfx_demo
