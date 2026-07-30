#pragma once

#include <cstdint>
#include <optional>

#include <antwika/gfx/IGfxBackend.hpp>
#include <antwika/gfx/WindowDesc.hpp>

#include "antwika/gfx_demo/DemoScene.hpp"

namespace antwika::gfx_demo
{

    using antwika::gfx::IGfxBackend;
    using antwika::gfx::WindowDesc;

    /**
     * @brief Opens a window, draws the scene into it, and closes it.
     *
     * The frame cap is optional, because the two ways this loop is used
     * want opposite things: a person watching a real window wants it to
     * stay up until they close it, and a headless run wants to end on
     * its own.
     * A backend is free to never report a close request -- which is
     * exactly what the null backend does -- so an uncapped run against
     * that backend never finishes.
     */
    class DemoLoop final
    {
    public:
        /**
         * @brief Construct the loop from its collaborators.
         * @param backend Supplies the window and its events.
         * @param scene Draws each frame.
         */
        DemoLoop(IGfxBackend &backend, const DemoScene &scene);

        DemoLoop(const DemoLoop &) = delete;
        DemoLoop(DemoLoop &&) = delete;

        DemoLoop &operator=(const DemoLoop &) = delete;
        DemoLoop &operator=(DemoLoop &&) = delete;

        /**
         * @brief Open a window and draw into it until it is done.
         *
         * Stops early if the backend reports a close request, and closes
         * the window on the way out either way.
         *
         * @param desc What the window should look like.
         * @param maxFrames How many frames to draw at most, or nullopt to
         * keep drawing for as long as the window stays open.
         * @throws antwika::gfx::GfxError If the window cannot be created.
         */
        void run(
            const WindowDesc &desc, std::optional<std::uint32_t> maxFrames);

    private:
        IGfxBackend &backend;
        const DemoScene &scene;
    };

} // namespace antwika::gfx_demo
