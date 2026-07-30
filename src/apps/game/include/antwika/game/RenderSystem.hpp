#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/game/Camera.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/GridScene.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/UiOverlay.hpp"

namespace antwika::game
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;
    using antwika::gfx::ITexture;
    using antwika::gfx::IWindow;

    /**
     * @brief Draws the grid into a window, once per tick.
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
     * pan rather than to the canvas centre.
     */
    class RenderSystem final : public ISystem
    {
    public:
        /**
         * @brief Construct the system over what it draws and reads.
         * @param window Window whose renderer receives each frame.
         * @param scene Turns a snapshot into drawing calls.
         * @param atlas The texture every tile is blitted from; it must
         * have come from this window's renderer, and must outlive this
         * system.
         * @param paths Read for the path cells.
         * @param camera Read for where to draw from.
         * @param extent Read for the bounds to draw within.
         * @param overlay Read for the toolbar's picture, painted last.
         */
        RenderSystem(
            IWindow &window,
            const GridScene &scene,
            const ITexture &atlas,
            const PathIndex &paths,
            const Camera &camera,
            GridExtent extent,
            const UiOverlay &overlay);

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
        IWindow &window;
        const GridScene &scene;
        const ITexture &atlas;
        const PathIndex &paths;
        const Camera &camera;
        GridExtent extent;
        const UiOverlay &overlay;
    };

} // namespace antwika::game
