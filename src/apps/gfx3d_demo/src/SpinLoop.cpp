#include "antwika/gfx3d_demo/SpinLoop.hpp"

#include <cstdint>
#include <optional>

#include <antwika/app/WindowEvents.hpp>
#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/IWindow.hpp>

namespace antwika::gfx3d_demo
{


    SpinLoop::SpinLoop(
        IGfxBackend &backend,
        const SpinScene &scene,
        ISleeper &sleeper,
        std::chrono::milliseconds framePeriod)
        : backend(backend),
          scene(scene),
          sleeper(sleeper),
          framePeriod(framePeriod)
    {
    }

    void SpinLoop::run(
        const WindowDesc &desc,
        const MeshData &cube,
        std::optional<std::uint32_t> maxFrames)
    {
        const auto window = backend.createWindow(desc);
        auto &flat = window->renderer();

        const auto mesh = flat.createMesh(cube);

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

            scene.draw(flat, *mesh, window->size(), tickCount);
            flat.present();

            sleeper.sleep(framePeriod);

            ++tickCount;
        }

        window->close();
    }

    std::uint64_t SpinLoop::ticks() const noexcept
    {
        return tickCount;
    }

}
