#pragma once

#include <antwika/console/ConsolePicture.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/Size.hpp>

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
     *
     * **The one place in this application reading the reported size**,
     * and it reads it to place the picture and nothing else: every
     * command is in canvas pixels, and a gfx::ViewportRenderer built
     * fresh each frame scales and centres them into whatever the window
     * currently is -- which is how F10's fullscreen enlarges the editor
     * rather than parking it in a corner.  The remainder is pillarboxed,
     * exactly as apps/game draws, and the pointer is mapped back through
     * the same transform upstream of the recorder in main().
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
         * @param canvas The size the window was **asked** for, which is
         * what every drawn command is expressed in.
         * @param console The console's picture, painted last so the
         * sheet stands over the whole editor; empty while no console
         * is mounted, which paints nothing.  Must outlive this object.
         */
        RenderSink(
            IWindow &window,
            const EditorScene &scene,
            const EditorSink &editor,
            Size canvas,
            const antwika::console::ConsolePicture &console);

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
        Size canvas;
        const antwika::console::ConsolePicture &console;
    };

} // namespace antwika::music_editor
