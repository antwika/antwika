#pragma once

#include <cstdint>
#include <optional>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/IGfxBackend.hpp>
#include <antwika/gfx/WindowDesc.hpp>
#include <antwika/input/IInputBackend.hpp>

#include "antwika/gfx_demo/DemoScene.hpp"

namespace antwika::gfx_demo
{

    using antwika::gfx::Bitmap;
    using antwika::gfx::IGfxBackend;
    using antwika::gfx::WindowDesc;
    using antwika::input::IInputBackend;

    /**
     * @brief Opens a window, draws the scene into it, lets the pointer
     * press its buttons, and closes it.
     *
     * The frame cap is optional, because the two ways this loop is used
     * want opposite things: a person watching a real window wants it to
     * stay up until they close it, and a headless run wants to end on
     * its own.
     * A backend is free to never report a close request -- which is
     * exactly what the null backend does -- so an uncapped run against
     * that backend never finishes.
     *
     * The click count lives here rather than in the scene: what a button
     * did is the run's state, and a scene that held it could not stay a
     * pure function of what it is handed. There is no engine and no
     * replay in this demo, so a frame is the whole unit of time and the
     * window's reported size is a safe thing to lay out against.
     */
    class DemoLoop final
    {
    public:
        /**
         * @brief Construct the loop from its collaborators.
         * @param backend Supplies the window and its events.
         * @param input Supplies the keyboard and pointer edges.
         * @param scene Draws each frame.
         */
        DemoLoop(
            IGfxBackend &backend,
            IInputBackend &input,
            const DemoScene &scene);

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
         * @param logo Uploaded once, after the window exists, and drawn
         * into every frame.
         * @param maxFrames How many frames to draw at most, or nullopt to
         * keep drawing for as long as the window stays open.
         * @throws antwika::gfx::GfxError If the window or the texture
         * cannot be created.
         */
        void run(
            const WindowDesc &desc,
            const Bitmap &logo,
            std::optional<std::uint32_t> maxFrames);

        /**
         * @brief Get how many times the counting button has been pressed.
         * @return The count, which the reset button puts back to zero.
         */
        [[nodiscard]] std::uint32_t clicks() const noexcept;

    private:
        IGfxBackend &backend;
        IInputBackend &input;
        const DemoScene &scene;

        std::uint32_t clickCount = 0;
    };

} // namespace antwika::gfx_demo
