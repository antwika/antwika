#pragma once

#include <cstdint>
#include <optional>

#include <antwika/gfx/IGfxBackend.hpp>
#include <antwika/gfx/MeshData.hpp>
#include <antwika/gfx/WindowDesc.hpp>

#include "antwika/gfx3d_demo/SpinScene.hpp"

namespace antwika::gfx3d_demo
{

    using antwika::gfx::IGfxBackend;
    using antwika::gfx::MeshData;
    using antwika::gfx::WindowDesc;

    /**
     * @brief Opens a window, uploads the cube, turns it one tick per
     * frame, and closes it.
     *
     * The frame cap is optional for the reason gfx_demo's is: a person
     * watching a real window wants it to stay up until they close it,
     * and a headless run wants to end on its own.
     * A backend is free never to report a close -- which is exactly what
     * the null backend does -- so an uncapped run against that backend
     * never finishes.
     *
     * The tick count lives here rather than in the scene, so the scene
     * can stay a pure function of what it is handed.
     * It counts frames drawn, and it is the only thing the picture
     * moves with: no clock is read anywhere in this app.
     */
    class SpinLoop final
    {
    public:
        /**
         * @brief Construct the loop from its collaborators.
         * @param backend Supplies the window and its events.
         * @param scene Draws each frame.
         */
        SpinLoop(IGfxBackend &backend, const SpinScene &scene);

        SpinLoop(const SpinLoop &) = delete;
        SpinLoop(SpinLoop &&) = delete;

        SpinLoop &operator=(const SpinLoop &) = delete;
        SpinLoop &operator=(SpinLoop &&) = delete;

        /**
         * @brief Open a window and draw into it until it is done.
         *
         * Stops early if the backend reports a close request, and closes
         * the window on the way out either way.
         *
         * @param desc What the window should look like.
         * @param cube Uploaded once, after the window exists, and drawn
         * into every frame.
         * @param maxFrames How many frames to draw at most, or nullopt
         * to keep drawing for as long as the window stays open.
         * @throws antwika::gfx::GfxError If the window cannot be
         * created, if the geometry cannot be uploaded, or if the chosen
         * backend has no 3D renderer to draw it with.
         */
        void run(
            const WindowDesc &desc,
            const MeshData &cube,
            std::optional<std::uint32_t> maxFrames);

        /**
         * @brief Get how many frames have been drawn.
         * @return The count, which is also the tick the next frame will
         * be drawn at.
         */
        [[nodiscard]] std::uint64_t ticks() const noexcept;

    private:
        IGfxBackend &backend;
        const SpinScene &scene;

        std::uint64_t tickCount = 0;
    };

} // namespace antwika::gfx3d_demo
