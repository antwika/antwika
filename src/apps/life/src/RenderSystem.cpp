#include "antwika/life/RenderSystem.hpp"

#include "antwika/life/Board.hpp"

namespace antwika::life
{

    RenderSystem::RenderSystem(
        IWindow &window,
        const BoardScene &scene,
        std::uint32_t width,
        std::uint32_t height)
        : window(window), scene(scene), width(width), height(height)
    {
    }

    void RenderSystem::update(World &world, antwika::time::Tick)
    {
        const auto board = readBoardFromView(world, width, height);

        auto &renderer = window.renderer();
        scene.draw(renderer, window.size(), board);
        renderer.present();
    }

} // namespace antwika::life
