#include "antwika/game/RenderSystem.hpp"

#include <antwika/app/FramePresentation.hpp>
#include <antwika/app/PointerReading.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/ui/Hover.hpp>
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
        [[nodiscard]] antwika::ui::DrawList hovered(
            const UiOverlay &overlay,
            const antwika::input::PointerHintChannel &hint)
        {
            auto picture = overlay.commands();

            antwika::ui::applyHover(
                picture,
                overlay.hoverTargets(),
                antwika::app::hoverFrom(hint.forRenderingOnly()));

            return picture;

        } // GCOVR_EXCL_LINE

        constexpr antwika::gfx::Color kWorldBackdrop{
            .red = 10, .green = 12, .blue = 18};

        constexpr antwika::gfx::Color kSurround{
            .red = 0, .green = 0, .blue = 0};

        const DesirabilityField kNoField;
    }

    RenderSystem::RenderSystem(const RenderSetup &setup) : setup(setup)
    {
    }

    void RenderSystem::update(World &world, antwika::time::Tick)
    {
        latest = snapshotOf(
            world,
            setup.paths,
            setup.camera,
            setup.extent,
            setup.pause.paused());

        latest.plan = planFor();

        latest.view = setup.view.has_value()
            ? setup.view->get().view()
            : MapView::Normal;

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

        return planDrag(
            setup.overlay.tool(),
            drag.start(),
            drag.end(),
            setup.extent,
            setup.built);
    }

    void RenderSystem::draw(antwika::animation::Progress subTick)
    {
        antwika::app::presentViewport(
            setup.window,
            setup.canvas,
            kSurround,
            setup.consoleOverlay,
            [this, subTick](IRenderer &view)
            {
                if (setup.fps.has_value())
                {
                    setup.fps->get().record();
                }

                drawScreen(view, subTick);
            });
    }

    void RenderSystem::drawScreen(
        IRenderer &renderer, antwika::animation::Progress subTick)
    {
        if (setup.mode.mode() == AppMode::MainMenu)
        {
            setup.menuScene.draw(
                renderer, hovered(setup.menuOverlay, setup.hint));
            return;
        }

        if (setup.mode.mode() == AppMode::SaveLoad)
        {
            setup.saveScene.draw(
                renderer, hovered(setup.saveOverlay, setup.hint));
            return;
        }

        if (setup.mode.mode() == AppMode::WorldMap)
        {
            renderer.clear(kWorldBackdrop);

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

        const auto covered = pointer.has_value()
            && setup.overlay.coversPoint(antwika::gfx::Point{
                .x = pointer->position.x, .y = pointer->position.y});

        latest.ghost = ghostFor(
            pointer,
            setup.camera,
            setup.extent,
            setup.overlay.tool(),
            covered,
            setup.paths,
            setup.built);

        latest.hover = hoverFor(pointer, setup.camera, latest, covered);

        setup.scene.draw(
            renderer, setup.canvas, latest, setup.atlases, subTick);

        antwika::ui::paint(
            renderer, hovered(setup.overlay, setup.hint));

        if (setup.fps.has_value())
        {
            antwika::ui::paint(
                renderer,
                describeFps(
                    setup.canvas, setup.fps->get().perSecond()));
        }
    }

}
