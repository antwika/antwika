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

    class RenderSink final : public ITickEventSink
    {
    public:
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

        void handle(const TickEvent &event) override;

    private:
        IWindow &window;
        const EditorScene &scene;
        const EditorSink &editor;
        Size canvas;
        const antwika::console::ConsolePicture &console;
    };

}
