#include "antwika/gfx_demo/DemoLoop.hpp"

#include <optional>
#include <variant>

#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/WindowEvent.hpp>
#include <antwika/gfx/WindowId.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputState.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/input/Position.hpp>
#include <antwika/ui/Pointer.hpp>
#include <antwika/ui/WidgetId.hpp>

namespace antwika::gfx_demo
{

    using antwika::gfx::CloseRequested;
    using antwika::gfx::Point;
    using antwika::input::InputEvent;
    using antwika::input::InputState;
    using antwika::input::MouseButton;
    using antwika::input::PointerButtonPressed;
    using antwika::input::PointerButtonReleased;
    using antwika::input::PointerMoved;
    using antwika::input::Position;
    using antwika::ui::kNoWidget;
    using antwika::ui::Pointer;

    namespace
    {
        // input::Position and gfx::Point match field for field.
        // They stay unrelated types so input need not depend on gfx.
        // Deciding they mean the same thing is the application's job.
        // This is the application saying so.
        [[nodiscard]] Point asPoint(Position position) noexcept
        {
            return Point{.x = position.x, .y = position.y};
        }

        // Until something says where it is, the pointer is nowhere.
        // The folded default would put it in the window's corner.
        // A widget can be in that corner, and would look hovered.
        [[nodiscard]] bool locates(const InputEvent &event) noexcept
        {
            return std::holds_alternative<PointerMoved>(event)
                   || std::holds_alternative<PointerButtonPressed>(event)
                   || std::holds_alternative<PointerButtonReleased>(event);
        }
    } // namespace

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

            const auto &mouse = state.mouse();
            const Pointer pointer{
                .position =
                    located ? std::optional<Point>{asPoint(mouse.position())}
                            : std::nullopt,
                .down = mouse.isDown(MouseButton::Left),
                .pressed = mouse.wasPressed(MouseButton::Left)};

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
