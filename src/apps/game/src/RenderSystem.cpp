#include "antwika/game/RenderSystem.hpp"

#include <antwika/gfx/Color.hpp>
#include <antwika/ui/Painter.hpp>

#include "antwika/game/SceneSnapshot.hpp"

namespace antwika::game
{

    namespace
    {
        // Around the world map rather than behind it.
        // The map covers only the tiles it has, being centred.
        // A mode owns the whole screen, so the rest is filled here.
        constexpr antwika::gfx::Color kWorldBackdrop{
            .red = 10, .green = 12, .blue = 18};
    } // namespace

    RenderSystem::RenderSystem(const RenderSetup &setup) : setup(setup)
    {
    }

    void RenderSystem::update(World &world, antwika::time::Tick)
    {
        auto &renderer = setup.window.renderer();

        if (setup.mode.mode() == AppMode::MainMenu)
        {
            // The whole screen, with no grid behind it.
            // The menu is a mode rather than a modal -- see AppMode.hpp.
            setup.menuScene.draw(renderer, setup.menuOverlay.commands());
            renderer.present();
            return;
        }

        if (setup.mode.mode() == AppMode::WorldMap)
        {
            renderer.clear(kWorldBackdrop);

            // Against the configured canvas, not the reported size.
            // That is the layout WorldMapSink resolves a click against.
            setup.worldScene.draw(
                renderer,
                setup.canvas,
                worldSnapshotOf(setup.cities.world()));
            renderer.present();
            return;
        }

        drawGrid(world);
        renderer.present();
    }

    void RenderSystem::drawGrid(const World &world)
    {
        auto &renderer = setup.window.renderer();

        setup.scene.draw(
            renderer,
            setup.window.size(),
            snapshotOf(
                world, setup.paths, setup.camera, setup.extent),
            setup.atlas);

        // Over the grid, and last, so the bar reads as being in front.
        // Laid out against the size the window was asked for.
        // That is the size UiSink described it against.
        // And the size a recorded click was resolved against.
        antwika::ui::paint(renderer, setup.overlay.commands());
    }

} // namespace antwika::game
