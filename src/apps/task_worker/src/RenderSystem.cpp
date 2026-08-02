#include "antwika/task_worker/RenderSystem.hpp"

#include "antwika/task_worker/PoolSnapshot.hpp"

namespace antwika::task_worker
{

    RenderSystem::RenderSystem(
        IWindow &window,
        const PoolScene &scene,
        const TaskRegistry &registry)
        : window(window), scene(scene), registry(registry)
    {
    }

    void RenderSystem::update(World &world, antwika::time::Tick tick)
    {
        const auto snapshot = snapshotOf(world, registry, tick);

        auto &renderer = window.renderer();
        scene.draw(renderer, window.configuredSize(), snapshot);
        renderer.present();
    }

} // namespace antwika::task_worker
