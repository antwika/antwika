#include "antwika/gfx_demo/DemoLoop.hpp"

#include <optional>
#include <variant>

#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/WindowEvent.hpp>
#include <antwika/gfx/WindowId.hpp>

namespace antwika::gfx_demo
{

    using antwika::gfx::CloseRequested;

    DemoLoop::DemoLoop(IGfxBackend &backend, const DemoScene &scene)
        : backend(backend), scene(scene)
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

            scene.draw(window->renderer(), window->size(), *texture);
            window->renderer().present();
        }

        window->close();
    }

} // namespace antwika::gfx_demo
