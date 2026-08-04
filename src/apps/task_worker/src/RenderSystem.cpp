#include "antwika/task_worker/RenderSystem.hpp"

#include <antwika/app/FramePresentation.hpp>

#include "antwika/task_worker/PoolSnapshot.hpp"

namespace antwika::task_worker
{

    RenderSystem::RenderSystem(
        IWindow &window,
        const PoolScene &scene,
        const TaskRegistry &registry,
        std::optional<std::reference_wrapper<
            const antwika::console::ConsolePicture>>
            consoleOverlay)
        : window(window),
          scene(scene),
          registry(registry),
          consoleOverlay(consoleOverlay)
    {
    }

    void RenderSystem::update(World &world, antwika::time::Tick tick)
    {
        const auto snapshot = snapshotOf(world, registry, tick);

        // The console last of all, over the pool.
        // Described in the tick path, painted here only.
        antwika::app::presentFrame(
            window,
            consoleOverlay,
            [this, &snapshot](antwika::gfx::IRenderer &renderer)
            {
                scene.draw(renderer, window.configuredSize(), snapshot);
            });
    }

} // namespace antwika::task_worker
