#pragma once

#include <cstdint>
#include <functional>
#include <optional>

#include <antwika/console/ConsolePicture.hpp>
#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/life/BoardScene.hpp"

namespace antwika::life
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;
    using antwika::gfx::IWindow;

    class RenderSystem final : public ISystem
    {
    public:
        RenderSystem(
            IWindow &window,
            const BoardScene &scene,
            std::uint32_t width,
            std::uint32_t height,
            std::optional<std::reference_wrapper<
                const antwika::console::ConsolePicture>>
                console = std::nullopt);

        RenderSystem(const RenderSystem &) = delete;
        RenderSystem(RenderSystem &&) = delete;

        RenderSystem &operator=(const RenderSystem &) = delete;
        RenderSystem &operator=(RenderSystem &&) = delete;

        void update(World &world, antwika::time::Tick tick) override;

    private:
        IWindow &window;
        const BoardScene &scene;
        std::uint32_t width;
        std::uint32_t height;
        std::optional<std::reference_wrapper<
            const antwika::console::ConsolePicture>>
            console;
    };

}
