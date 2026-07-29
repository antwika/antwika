#pragma once

#include <cstdint>

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
     * Frame-capped rather than open-ended, because a backend is free to
     * never report a close request -- which is exactly what the headless
     * null backend does, so an uncapped loop would hang CI.
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
         * @param maxFrames How many frames to draw at most.
         * @throws antwika::gfx::GfxError If the window cannot be created.
         */
        void run(const WindowDesc &desc, std::uint32_t maxFrames);

    private:
        IGfxBackend &backend;
        const DemoScene &scene;
    };

} // namespace antwika::gfx_demo
