#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/game/AppMode.hpp"
#include "antwika/game/Camera.hpp"
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

        /** @brief Read for where the live city is drawn from. */
        const Camera &camera;

        /** @brief Read for the bounds to draw within. */
        GridExtent extent;

        /** @brief Read for the toolbar's picture, painted last. */
        const UiOverlay &overlay;

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
     */
    class RenderSystem final : public ISystem
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
         * @brief Draw the world's current state and present the frame.
         * @param world World read from; never written to.
         * @param tick The tick this frame is for; unused.
         */
        void update(World &world, antwika::time::Tick tick) override;

    private:
        void drawGrid(const World &world);

        RenderSetup setup;
    };

} // namespace antwika::game
