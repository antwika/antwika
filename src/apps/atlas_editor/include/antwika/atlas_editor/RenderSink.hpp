#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>

#include <antwika/console/ConsolePicture.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/time/ISleeper.hpp>

#include "antwika/atlas_editor/EditorScene.hpp"
#include "antwika/atlas_editor/EditorState.hpp"
#include "antwika/atlas_editor/UiOverlay.hpp"

namespace antwika::atlas_editor
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::gfx::ITexture;
    using antwika::gfx::IWindow;
    using antwika::time::ISleeper;

    /**
     * @brief Draws the sheet and the toolbar, once per engine.tick.
     *
     * Rendering hangs off the tick loop without feeding back into it:
     * everything it reads arrives as an immutable SceneSnapshot and a
     * DrawList somebody else described, and nothing it does is visible
     * to any other sink.
     * Registered last, after EditorSink, so the frame is of the state
     * the tick ended with.
     *
     * **The uploaded texture is the one piece of render-side state
     * here**, and it is safe for one reason: it is a copy of the canvas
     * taken when the canvas last changed, so it can only ever be the
     * sheet or a slightly older sheet, and nothing reads it back.
     * A pixel is not re-uploaded per edit either -- a stroke that paints
     * one pixel ten times leaves the revision alone, since Canvas::set
     * calls writing the colour already there no change at all.
     *
     * **What "changed" means is the revision *and* the count of loads**,
     * and the second half is not decoration. EditorState::replace()
     * installs a whole new Canvas, which begins at revision zero -- so a
     * load on a session that has painted nothing moves the revision from
     * zero to zero, and a key made of the revision alone would skip the
     * upload and go on drawing the sheet that is no longer open.
     * Picking up a file something else changed is most of why anybody
     * presses load with nothing unsaved, which is exactly the case that
     * would have lied.
     */
    class RenderSink final : public ITickEventSink
    {
    public:
        /**
         * @brief Construct the sink over everything it draws from.
         * @param window Window whose renderer receives each frame. Must
         * outlive this sink.
         * @param scene Draws the sheet. Must outlive this sink.
         * @param state Snapshotted and uploaded each tick. Must outlive
         * this sink.
         * @param overlay Holds the toolbar's picture. Must outlive this
         * sink.
         * @param console Holds the debug console's picture, painted
         * over everything else. Must outlive this sink.
         * @param sleeper Paces the frames. Must outlive this sink.
         * @param framePeriod How long to hold each frame.
         */
        RenderSink(
            IWindow &window,
            const EditorScene &scene,
            const EditorState &state,
            const UiOverlay &overlay,
            const antwika::console::ConsolePicture &console,
            ISleeper &sleeper,
            std::chrono::milliseconds framePeriod);

        RenderSink(const RenderSink &) = delete;
        RenderSink(RenderSink &&) = delete;

        RenderSink &operator=(const RenderSink &) = delete;
        RenderSink &operator=(RenderSink &&) = delete;

        /**
         * @brief Draw a frame if this is a tick.
         * @param event The event to fold in; anything but engine.tick is
         * ignored.
         * @throws antwika::gfx::GfxError If the renderer will not hold
         * the sheet -- a sheet this window cannot draw is worth ending
         * the run over, since every frame after it would be a lie.
         */
        void handle(const TickEvent &event) override;

    private:
        // Which sheet the texture in hand was made from.
        // Two numbers rather than one, for the reason above.
        struct UploadKey final
        {
            std::uint64_t revision = 0;
            std::uint32_t loads = 0;

            [[nodiscard]] bool operator==(const UploadKey &other) const
                = default;
        };

        void uploadIfChanged();

        IWindow &window;
        const EditorScene &scene;
        const EditorState &state;
        const UiOverlay &overlay;
        const antwika::console::ConsolePicture &console;
        ISleeper &sleeper;
        std::chrono::milliseconds framePeriod;

        std::unique_ptr<ITexture> sheet;
        std::optional<UploadKey> uploaded;
    };

} // namespace antwika::atlas_editor
