#include "antwika/gfx3d_demo/SpinLoop.hpp"

#include <cstdint>
#include <optional>
#include <variant>

#include <antwika/gfx/GfxError.hpp>
#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/IRenderer3D.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/WindowEvent.hpp>

namespace antwika::gfx3d_demo
{

    using antwika::gfx::CloseRequested;
    using antwika::gfx::GfxError;

    SpinLoop::SpinLoop(IGfxBackend &backend, const SpinScene &scene)
        : backend(backend), scene(scene)
    {
    }

    void SpinLoop::run(
        const WindowDesc &desc,
        const MeshData &cube,
        std::optional<std::uint32_t> maxFrames)
    {
        const auto window = backend.createWindow(desc);
        auto &flat = window->renderer();

        // A backend need not have a 3D path at all.
        // IRenderer::renderer3d() returns null when it has none.
        // That is a question a caller can ask.
        // A call that quietly did nothing would not be.
        // This whole application is one mesh.
        // So there is nothing left to carry on with here.
        // Saying so beats drawing an empty window.
        auto *space = flat.renderer3d();

        if (space == nullptr)
        {
            throw GfxError(
                "gfx3d_demo: the selected graphics backend draws no 3D");
        }

        // After the window, since a backend may have no device yet.
        // Declared after it too, so it is destroyed first.
        const auto mesh = space->createMesh(cube);

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

            scene.draw(flat, *space, *mesh, window->size(), tickCount);
            flat.present();

            ++tickCount;
        }

        window->close();
    }

    std::uint64_t SpinLoop::ticks() const noexcept
    {
        return tickCount;
    }

} // namespace antwika::gfx3d_demo
