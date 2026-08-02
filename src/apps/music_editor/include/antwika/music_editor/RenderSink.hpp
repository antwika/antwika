#pragma once

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/IWindow.hpp>

#include "antwika/music_editor/EditorScene.hpp"
#include "antwika/music_editor/EditorSink.hpp"

namespace antwika::music_editor
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::gfx::IWindow;

    /**
     * @brief Draws the editor, once per engine.tick.
     *
     * Registered after EditorSink, so a frame is of the state the tick
     * ended with.
     *
     * **It does no pacing of its own**, unlike ui_demo's: this run is
     * paced by how much audio the device has taken, and a sleeper here
     * would be a second opinion about how fast the run goes.
     */
    class RenderSink final : public ITickEventSink
    {
    public:
        /**
         * @brief Construct the sink over everything it draws from.
         * @param window Whose renderer receives each frame; must outlive
         * this object.
         * @param scene Draws the picture; must outlive this object.
         * @param editor Holds the picture; must outlive this object.
         */
        RenderSink(
            IWindow &window,
            const EditorScene &scene,
            const EditorSink &editor);

        RenderSink(const RenderSink &) = delete;
        RenderSink(RenderSink &&) = delete;

        RenderSink &operator=(const RenderSink &) = delete;
        RenderSink &operator=(RenderSink &&) = delete;

        /**
         * @brief Draw one frame, on the tick itself.
         * @param event The event, drawn on only when it is the tick.
         */
        void handle(const TickEvent &event) override;

    private:
        IWindow &window;
        const EditorScene &scene;
        const EditorSink &editor;
    };

} // namespace antwika::music_editor
