#include "antwika/life/RenderSystem.hpp"

#include <antwika/app/FramePresentation.hpp>

#include "antwika/life/Board.hpp"

namespace antwika::life
{

    RenderSystem::RenderSystem(
        IWindow &window,
        const BoardScene &scene,
        std::uint32_t width,
        std::uint32_t height,
        std::optional<std::reference_wrapper<
            const antwika::console::ConsolePicture>>
            console)
        : window(window),
          scene(scene),
          width(width),
          height(height),
          console(console)
    {
    }

    void RenderSystem::update(World &world, antwika::time::Tick)
    {
        const auto board = readBoardFromView(world, width, height);

        // Described in the tick path like the board, painted here only.
        antwika::app::presentFrame(
            window,
            console,
            [this, &board](antwika::gfx::IRenderer &renderer)
            {
                scene.draw(renderer, window.configuredSize(), board);
            });
    }

} // namespace antwika::life
