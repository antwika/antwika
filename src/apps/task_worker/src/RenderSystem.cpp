#include "antwika/task_worker/RenderSystem.hpp"

#include <antwika/ui/Painter.hpp>

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

        auto &renderer = window.renderer();
        scene.draw(renderer, window.configuredSize(), snapshot);

        // The console last of all, over the pool.
        // Described in the tick path, painted here only.
        if (consoleOverlay.has_value())
        {
            antwika::ui::paint(
                renderer, consoleOverlay->get().commands());
        }

        renderer.present();
    }

} // namespace antwika::task_worker
