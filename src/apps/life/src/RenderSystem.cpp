#include "antwika/life/RenderSystem.hpp"

#include <antwika/ui/Painter.hpp>

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

        auto &renderer = window.renderer();
        scene.draw(renderer, window.configuredSize(), board);

        // Described in the tick path like the board, painted here only.
        if (console.has_value())
        {
            antwika::ui::paint(renderer, console->get().commands());
        }

        renderer.present();
    }

} // namespace antwika::life
