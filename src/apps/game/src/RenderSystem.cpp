#include "antwika/game/RenderSystem.hpp"

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/ViewportRenderer.hpp>
#include <antwika/ui/Painter.hpp>

#include "antwika/game/BuildGhost.hpp"
#include "antwika/game/FpsReadout.hpp"
#include "antwika/game/Hover.hpp"
#include "antwika/game/OverlayField.hpp"
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

        // What is left over when the window is not the canvas's shape.
        // Painted after the picture rather than before it.
        // So a tile reaching past the canvas's edge is covered by it.
        constexpr antwika::gfx::Color kSurround{
            .red = 0, .green = 0, .blue = 0};
    } // namespace

    RenderSystem::RenderSystem(const RenderSetup &setup) : setup(setup)
    {
    }

    void RenderSystem::update(World &world, antwika::time::Tick)
    {
        // Taken unconditionally, even in a mode that draws no grid.
        // So there is no branch here to want a mode combination for.
        // The pause is read here rather than in draw().
        // A frame between two ticks then draws the tick's own answer.
        // Which is the rule the whole snapshot exists to keep.
        latest = snapshotOf(
            world,
            setup.paths,
            setup.camera,
            setup.extent,
            setup.pause.paused());

        // Beside the snapshot rather than beside the ghost.
        // Because it is simulation state.
        // RoadDrag is written inside the tick path from recorded input.
        // So a replay works the same route out.
        // Once a tick is therefore enough.
        // Nothing between two ticks can change what it would lay.
        latest.plan = planFor();

        // Beside the plan, and once a tick for its reason exactly.
        // Which view is showing is written inside the tick path.
        // And what it paints is derived from what the world holds.
        // So no frame can see anything here the tick did not.
        latest.view = setup.view.has_value()
            ? setup.view->get().view()
            : MapView::Normal;

        // A field to paint from where there is one to paint.
        // Absent leaves the desirability view painting nothing.
        // Which is the same shape every optional member here has.
        static const DesirabilityField kNoField;
        latest.overlay = overlayFieldOf(
            world,
            latest.view,
            setup.desirability.has_value() ? setup.desirability->get()
                                           : kNoField,
            setup.extent);

        draw(antwika::animation::Progress());
    }

    RoadPlan RenderSystem::planFor() const
    {
        if (!setup.drag.has_value() || !setup.drag->get().active())
        {
            return RoadPlan{};
        }

        const auto &drag = setup.drag->get();

        return planRoad(
            drag.start(), drag.end(), setup.extent, setup.built);
    }

    void RenderSystem::draw(antwika::animation::Progress subTick)
    {
        // **The one place in this application reading the reported size.**
        // And it reads it to place a picture and nothing else.
        // Every call below is in canvas pixels, exactly as before.
        // This scales and centres them, after every decision is made.
        // A new one each frame, so a resize needs no handling of its own.
        antwika::gfx::ViewportRenderer view(
            setup.window.renderer(), setup.window.size(), setup.canvas);

        // Counted here rather than in update().
        // This is the one thing that runs exactly once per frame.
        // update() draws the tick's own frame by calling it.
        // app::FramePacedSource calls it again for each frame between.
        // The count goes nowhere but the readout below.
        if (setup.fps.has_value())
        {
            setup.fps->get().record();
        }

        drawMode(view, subTick);

        // Last, so whatever reached past the canvas is covered.
        // Nothing at all when the window is the canvas's own shape.
        view.fillSurround(kSurround);
        view.present();
    }

    void RenderSystem::drawMode(
        IRenderer &renderer, antwika::animation::Progress subTick)
    {
        if (setup.mode.mode() == AppMode::MainMenu)
        {
            // The whole screen, with no grid behind it.
            // The menu is a mode rather than a modal -- see AppMode.hpp.
            setup.menuScene.draw(renderer, setup.menuOverlay.commands());
            return;
        }

        if (setup.mode.mode() == AppMode::SaveLoad)
        {
            setup.saveScene.draw(renderer, setup.saveOverlay.commands());
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
            return;
        }

        drawGrid(renderer, subTick);
    }

    void RenderSystem::drawGrid(
        IRenderer &renderer, antwika::animation::Progress subTick)
    {
        const auto pointer = setup.hint.forRenderingOnly();

        // Asked of the layout, at the position the ghost is drawn from.
        // Rather than of the flag the last recorded press settled.
        // The two differ while the pointer moves with nothing held.
        // Which is exactly when a ghost is worth drawing.
        // Idle motion is thinned out of the recorded stream.
        // So the flag stays true from the click that chose a tool.
        // Until the next press, and the ghost was hidden for all of it.
        // What the bar covers still comes *from* UiOverlay.
        // Which is the direction the rule allows.
        // The hint decides this and nothing else.
        const auto covered = pointer.has_value()
            && setup.overlay.coversPoint(antwika::gfx::Point{
                .x = pointer->position.x, .y = pointer->position.y});

        // Worked out here rather than staged into the World.
        // The hint is a value no replay reproduces.
        // Folding it in would make the two disagree -- see BuildGhost.
        // Re-read every frame rather than once a tick.
        // So the ghost follows the pointer at the rate it is drawn at.
        // That is free, since a hint is render-side by construction.
        latest.ghost = ghostFor(
            pointer,
            setup.camera,
            setup.extent,
            setup.overlay.tool(),
            covered,
            setup.paths,
            setup.built);

        // Off the same channel and under the same rule as the ghost.
        // Read against the snapshot this frame is about to draw.
        // So the panel and what is on screen are the one picture.
        // Worked out here rather than in a sink, for the ghost's reason.
        // No replay reproduces a hint.
        // So nothing folded from one may reach what a replay does.
        latest.hover = hoverFor(pointer, setup.camera, latest, covered);

        // Against the configured canvas, never the reported size.
        // The scene's own culling is then the same on every window.
        // And what the viewport does to it is a scale, applied after.
        setup.scene.draw(
            renderer, setup.canvas, latest, setup.atlases, subTick);

        // Over the grid, and last, so the bar reads as being in front.
        // Laid out against the size the window was asked for.
        // That is the size UiSink described it against.
        // And the size a recorded click was resolved against.
        antwika::ui::paint(renderer, setup.overlay.commands());

        // Described here rather than in a sink.
        // Everything else painted in this app is described in one.
        // The number is off a wall clock, which no tick may see.
        // On the city's screen alone, since a mode owns the screen.
        if (setup.fps.has_value())
        {
            antwika::ui::paint(
                renderer,
                describeFps(
                    setup.canvas, setup.fps->get().perSecond()));
        }
    }

} // namespace antwika::game
