#pragma once

#include <functional>
#include <optional>

#include <antwika/animation/Progress.hpp>
#include <antwika/app/IFramePass.hpp>
#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/PointerHintChannel.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/game/AppMode.hpp"
#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/FrameMeter.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/GridScene.hpp"
#include "antwika/game/MainMenuScene.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/SaveLoadScene.hpp"
#include "antwika/game/UiOverlay.hpp"
#include "antwika/game/WorldMapScene.hpp"
#include "antwika/game/WorldMapState.hpp"

namespace antwika::game
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;
    using antwika::gfx::ITexture;
    using antwika::gfx::IWindow;
    using antwika::gfx::Size;

    /**
     * @brief Everything the renderer draws each mode's picture out of.
     *
     * A struct with designated initialisers rather than a parameter list,
     * for the reason GameConfig gives: one screen per mode means one
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
         * @brief The area every mode is laid out against.
         *
         * The size the window was *asked* for, never the size one
         * reports -- see UiCanvas.hpp. The world map is centred in it,
         * and WorldMapSink resolves a click against the same number.
         */
        Size canvas;

        /** @brief Turns a grid snapshot into drawing calls. */
        const GridScene &scene;

        /**
         * @brief The texture every tile is blitted from.
         *
         * It must have come from this window's renderer.
         */
        const ITexture &atlas;

        /** @brief Read for the live city's path cells. */
        const PathIndex &paths;

        /** @brief Which cells hold a building, for the ghost. */
        const BuildingIndex &built;

        /** @brief Read for where the live city is drawn from. */
        const Camera &camera;

        /** @brief Read for the bounds to draw within. */
        GridExtent extent;

        /** @brief Read for the toolbar's picture, painted last. */
        const UiOverlay &overlay;

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
     * The window's size is read afresh every tick, so a resize needs no
     * handling of its own. That size reaches nothing but the culling test
     * and the drawing calls, which is what keeps a resize from perturbing
     * the simulation -- and why the projection is anchored to the camera's
     * pan rather than to the canvas centre. The *world map* is laid out
     * against the configured canvas instead, never the reported size, for
     * the reason the toolbar is: a click on a city is resolved against
     * that layout.
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
        void drawGrid(antwika::animation::Progress subTick);

        RenderSetup setup;

        // What the last update() saw, redrawn by every frame after it.
        // Written in update() and nowhere else.
        SceneSnapshot latest;
    };

} // namespace antwika::game
