#include "antwika/music_editor/RenderSink.hpp"

#include <antwika/app/FramePresentation.hpp>
#include <antwika/gfx/Color.hpp>

namespace antwika::music_editor
{

    namespace
    {
        constexpr antwika::gfx::Color kSurround{
            .red = 8, .green = 8, .blue = 10, .alpha = 255};
    }

    RenderSink::RenderSink(
        IWindow &window,
        const EditorScene &scene,
        const EditorSink &editor,
        const Size canvas,
        const antwika::console::ConsolePicture &console)
        : window(window),
          scene(scene),
          editor(editor),
          canvas(canvas),
          console(console)
    {
    }

    void RenderSink::handle(const TickEvent &event)
    {
        if (!antwika::app::drawsOn(event, window))
        {
            return;
        }

        antwika::app::presentViewport(
            window,
            canvas,
            kSurround,
            console,
            [this](antwika::gfx::IRenderer &view)
            {
                scene.draw(view, editor.commands());
            });
    }

}
