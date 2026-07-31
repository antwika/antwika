#include "antwika/game/RenderSystem.hpp"

#include <antwika/gfx/Color.hpp>
#include <antwika/ui/Painter.hpp>

#include "antwika/game/BuildGhost.hpp"
#include "antwika/game/Hover.hpp"
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
        // Taken unconditionally, even in a mode that draws no grid.
        // So there is no branch here to want a mode combination for.
        latest = snapshotOf(world, setup.paths, setup.camera, setup.extent);

        draw(antwika::animation::Progress());
    }

    void RenderSystem::draw(antwika::animation::Progress subTick)
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

        if (setup.mode.mode() == AppMode::SaveLoad)
        {
            setup.saveScene.draw(renderer, setup.saveOverlay.commands());
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

        drawGrid(subTick);
        renderer.present();
    }

    void RenderSystem::drawGrid(antwika::animation::Progress subTick)
    {
        auto &renderer = setup.window.renderer();

        // Worked out here rather than staged into the World.
        // The hint is a value no replay reproduces.
        // Folding it in would make the two disagree -- see BuildGhost.
        // What the bar covers comes *from* UiOverlay, never the reverse.
        // Re-read every frame rather than once a tick.
        // So the ghost follows the pointer at the rate it is drawn at.
        // That is free, since a hint is render-side by construction.
        latest.ghost = ghostFor(
            setup.hint.forRenderingOnly(),
            setup.camera,
            setup.extent,
            setup.overlay.tool(),
            setup.overlay.pointerOverUi(),
            setup.paths,
            setup.built);

        // Off the same channel and under the same rule as the ghost.
        // Read against the snapshot this frame is about to draw.
        // So the panel and what is on screen are the one picture.
        // Worked out here rather than in a sink, for the ghost's reason.
        // No replay reproduces a hint.
        // So nothing folded from one may reach what a replay does.
        latest.hover = hoverFor(
            setup.hint.forRenderingOnly(),
            setup.camera,
            latest,
            setup.overlay.pointerOverUi());

        setup.scene.draw(
            renderer, setup.window.size(), latest, setup.atlas, subTick);

        // Over the grid, and last, so the bar reads as being in front.
        // Laid out against the size the window was asked for.
        // That is the size UiSink described it against.
        // And the size a recorded click was resolved against.
        antwika::ui::paint(renderer, setup.overlay.commands());
    }

} // namespace antwika::game
