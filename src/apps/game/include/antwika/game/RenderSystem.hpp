#pragma once

#include <functional>
#include <optional>

#include <antwika/animation/Progress.hpp>
#include <antwika/app/IFramePass.hpp>
#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/PointerHintChannel.hpp>
#include <antwika/time/Tick.hpp>

#include <antwika/console/ConsolePicture.hpp>

#include "antwika/game/AppMode.hpp"
#include "antwika/game/AtlasTextures.hpp"
#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/FrameMeter.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/Desirability.hpp"
#include "antwika/game/GridScene.hpp"
#include "antwika/game/MainMenuScene.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/MapView.hpp"
#include "antwika/game/PauseState.hpp"
#include "antwika/game/RoadDrag.hpp"
#include "antwika/game/RoadPlan.hpp"
#include "antwika/game/SaveLoadScene.hpp"
#include "antwika/game/UiOverlay.hpp"
#include "antwika/game/WorldMapScene.hpp"
#include "antwika/game/WorldMapState.hpp"

namespace antwika::game
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;
    using antwika::gfx::IWindow;
    using antwika::gfx::Size;

    /**
     * @brief Everything the renderer draws each mode's picture out of.
     *
     * A struct with designated initialisers rather than a parameter list,
     * for the reason GameWiring gives: one screen per mode means one
     * scene and one overlay per mode, and a positional list of them is a
     * row of same-typed references distinguishable only by where they
     * sit.
     *
     * Every member is borrowed and must outlive the RenderSystem.
     */
    struct RenderSetup
    {
        /** @brief Window whose renderer receives each frame. */
        IWindow &window;

        /** @brief Which mode's picture to draw; only ever read. */
        const AppModeState &mode;

        /**
         * @brief The area every mode is drawn and laid out against.
         *
         * The size the window was *asked* for, never the size one
         * reports -- see UiCanvas.hpp. Every drawing call this system
         * makes is in these pixels, including the grid's: what the
         * reported size decides is only how big that picture is blitted
         * and where, through the gfx::ViewportRenderer built each
         * frame. The world map is centred in it, and WorldMapSink
         * resolves a click against the same number.
         */
        Size canvas;

        /** @brief Turns a grid snapshot into drawing calls. */
        const GridScene &scene;

        /**
         * @brief The three sheets every sprite is blitted from.
         *
         * Every one must have come from this window's renderer.
         */
        AtlasTextures atlases;

        /** @brief Read for the live city's path cells. */
        const PathIndex &paths;

        /** @brief Which cells hold a building, for the ghost. */
        const BuildingIndex &built;

        /** @brief Read for where the live city is drawn from. */
        const Camera &camera;

        /** @brief Read for the bounds to draw within. */
        GridExtent extent;

        /**
         * @brief Read for whether the run is being held still.
         *
         * Only ever read, exactly as the camera and the mode are: it is
         * simulation state a snapshot copies, and what it decides is
         * whether a walker is drawn moving between two ticks.
         */
        const PauseState &pause;

        /** @brief Read for the toolbar's picture, painted last. */
        const UiOverlay &overlay;

        /**
         * @brief Read for which picture of the city is showing.
         *
         * Only ever read, exactly as the pause is, and simulation state
         * on the same terms -- see MapView.
         *
         * Optional, and absent means the city itself with nothing
         * painted over it, which spares every test whose subject is
         * some other part of the picture.
         */
        std::optional<std::reference_wrapper<const MapViewState>> view =
            std::nullopt;

        /**
         * @brief Read for what the desirability view paints.
         *
         * The one overlay that is genuinely per cell rather than per
         * building, so it is the one that needs the field the serve
         * phase rebuilt rather than only the world.
         *
         * Optional on the view's terms exactly: absent leaves that one
         * view painting nothing, and every other view unaffected.
         */
        std::optional<std::reference_wrapper<const DesirabilityField>>
            desirability = std::nullopt;

        /**
         * @brief Read for the run of road being dragged out, if one is.
         *
         * Simulation state, exactly as the camera and the pause are, so
         * reading it here is reading a value a replay reproduces. That
         * is what makes the preview it draws unlike the ghost beside it,
         * which comes off a channel no replay reproduces.
         *
         * Optional, and absent by default, so a run with no drag to draw
         * needs no state for one -- which spares every test whose
         * subject is some other part of the picture.
         */
        std::optional<std::reference_wrapper<const RoadDrag>> drag =
            std::nullopt;

        /**
         * @brief Where the pointer is, for the placement ghost.
         *
         * The one thing here a replay does not reproduce, which is why
         * it reaches a renderer and nothing else -- see BuildGhost and
         * input::PointerHintChannel.
         */
        const antwika::input::PointerHintChannel &hint;

        /** @brief Draws the main menu. */
        const MainMenuScene &menuScene;

        /** @brief Read for the menu's picture. */
        const UiOverlay &menuOverlay;

        /** @brief Draws the save/load screen. */
        const SaveLoadScene &saveScene;

        /** @brief Read for the save/load screen's picture. */
        const UiOverlay &saveOverlay;

        /**
         * @brief Read for the debug console's picture, painted last.
         *
         * Over whichever mode's screen is up, since the console
         * belongs to no mode and is the topmost thing the app shows.
         *
         * Optional, and absent by default, so a run with no console
         * pays nothing for one -- the same shape the map view has.
         */
        std::optional<std::reference_wrapper<
            const antwika::console::ConsolePicture>>
            consoleOverlay = std::nullopt;

        /** @brief Draws the world and its cities. */
        const WorldMapScene &worldScene;

        /** @brief Read for the world to draw, and which city is open. */
        const WorldMapState &cities;

        /**
         * @brief Counts the frames this system draws, if anybody asked.
         *
         * **The second thing here a replay does not reproduce**, and the
         * stronger case of the two: a pointer hint is a position a
         * replay simply lacks, while this is a wall clock, which says
         * how fast the machine is. It is written and read here and
         * nowhere else -- see FrameMeter.
         *
         * Optional, and absent by default, so a run that draws no HUD
         * needs no clock: every test whose subject is the picture is
         * spared one, and so is any caller that has no wall clock to
         * offer.
         */
        std::optional<std::reference_wrapper<FrameMeter>> fps =
            std::nullopt;
    };

    /**
     * @brief Draws whichever mode the app is in, once per tick.
     *
     * Which one that is, is simulation state it only reads -- see
     * AppMode.hpp. A mode owns the whole screen: the menu is not painted
     * over a grid that is still running, the grid is not painted under a
     * menu, and the world map is not painted beside either.
     *
     * An observer on the same terms as apps/life's RenderSystem: it only
     * reads, and knows nothing about the systems it shares a tick with.
     *
     * It never closes the window, and does not check whether the window is
     * still open. Window lifetime belongs to the composition root that
     * created it, and WindowInputSource holds an id rather than a window
     * so it cannot close one either. A guard that can never be false is a
     * branch the coverage gate would demand an impossible test for -- see
     * blog/012.
     *
     * The atlas is borrowed for the same reason the window is: it belongs
     * to whoever opened the window, since a texture belongs to the
     * renderer that made it and has to be destroyed before that renderer
     * goes. Uploading it here would put a resource behind an observer
     * that is only supposed to read.
     *
     * **Every picture here is drawn in canvas pixels and scaled on the
     * way out.** Each frame builds a gfx::ViewportRenderer over the
     * window's reported size and the configured canvas, and every scene,
     * every UI painter and every readout draws through that. So the
     * whole game is laid out, hit-tested and simulated against one fixed
     * canvas -- nothing inside the tick path learns what size the window
     * is -- and the reported size decides only how big the result is
     * blitted and where. That is docs/resizable-windows.md's offset,
     * generalised to an offset and a uniform scale, and it is safe for
     * the same reason: it is applied after every decision has already
     * been made, and it is never asked what a pixel means.
     *
     * The reported size is read afresh every frame, so a resize needs no
     * handling of its own, and app::WindowPointerMapping runs the same
     * transform backwards on a pointer position before anything records
     * it. Going fullscreen is therefore a picture that got bigger and a
     * hit target that did not move.
     *
     * It is also the only thing in this app that reads
     * input::PointerHintChannel, and that is where the channel's whole
     * safety condition is kept: what is read there decides what is
     * drawn, and nothing else.
     *
     * **It draws more often than the tick advances**, which is what lets
     * a walker slide between two cells rather than jump. update() takes
     * the snapshot and draws the tick's own frame; draw() redraws that
     * same snapshot part of the way through the tick, and is called by
     * app::FramePacedSource in the gap before the next tick's events are
     * read.
     *
     * **A held run is the exception**, and the snapshot is what carries
     * it: PauseState stops the ticks a walker steps on but not the
     * frames drawn between them, so the pause is read into every
     * snapshot and GridScene draws a held walker at its step's own
     * phase whatever fraction of a tick a frame falls at.
     *
     * The snapshot is kept between the two, and it is the only render-side
     * mutable state in this app. That is safe for one structural reason
     * rather than by promise: draw() has no World parameter, so a frame
     * between two ticks has nothing it could read a moving world from and
     * nothing it could write one to. Giving it one would quietly remove
     * the guarantee.
     */
    class RenderSystem final : public ISystem, public antwika::app::IFramePass
    {
    public:
        /**
         * @brief Construct the system over what it draws and reads.
         * @param setup Everything it draws each mode's picture out of.
         */
        explicit RenderSystem(const RenderSetup &setup);

        RenderSystem(const RenderSystem &) = delete;
        RenderSystem(RenderSystem &&) = delete;

        RenderSystem &operator=(const RenderSystem &) = delete;
        RenderSystem &operator=(RenderSystem &&) = delete;

        /**
         * @brief Take a snapshot of the world and draw the tick's frame.
         * @param world World read from; never written to.
         * @param tick The tick this frame is for; unused.
         */
        void update(World &world, antwika::time::Tick tick) override;

        /**
         * @brief Redraw the last snapshot part way through the tick.
         *
         * Called between two ticks, so it reads the snapshot update()
         * took and never the world, which may be part way through being
         * changed by the time this runs.
         *
         * @param subTick How far through the tick this frame falls.
         */
        void draw(antwika::animation::Progress subTick) override;

    private:
        void drawMode(
            antwika::gfx::IRenderer &renderer,
            antwika::animation::Progress subTick);

        void drawScreen(
            antwika::gfx::IRenderer &renderer,
            antwika::animation::Progress subTick);

        void drawGrid(
            antwika::gfx::IRenderer &renderer,
            antwika::animation::Progress subTick);

        [[nodiscard]] RoadPlan planFor() const;

        RenderSetup setup;

        // What the last update() saw, redrawn by every frame after it.
        // Written in update() and nowhere else.
        SceneSnapshot latest;
    };

} // namespace antwika::game
