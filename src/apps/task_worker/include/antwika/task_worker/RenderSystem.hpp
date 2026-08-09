#pragma once

#include <functional>
#include <optional>

#include <antwika/console/ConsolePicture.hpp>
#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/task_worker/PoolScene.hpp"
#include "antwika/task_worker/TaskRegistry.hpp"

namespace antwika::task_worker
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;
    using antwika::gfx::IWindow;

    class RenderSystem final : public ISystem
    {
    public:
        RenderSystem(
            IWindow &window,
            const PoolScene &scene,
            const TaskRegistry &registry,
            std::optional<std::reference_wrapper<
                const antwika::console::ConsolePicture>>
                consoleOverlay = std::nullopt);

        RenderSystem(const RenderSystem &) = delete;
        RenderSystem(RenderSystem &&) = delete;

        RenderSystem &operator=(const RenderSystem &) = delete;
        RenderSystem &operator=(RenderSystem &&) = delete;

        void update(World &world, antwika::time::Tick tick) override;

    private:
        IWindow &window;
        const PoolScene &scene;
        const TaskRegistry &registry;
        std::optional<std::reference_wrapper<
            const antwika::console::ConsolePicture>>
            consoleOverlay;
    };

}
