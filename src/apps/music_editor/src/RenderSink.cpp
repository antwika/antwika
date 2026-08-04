#include "antwika/music_editor/RenderSink.hpp"

#include <antwika/app/FramePresentation.hpp>
#include <antwika/gfx/Color.hpp>

namespace antwika::music_editor
{

    namespace
    {
        // What the pillarboxes are filled with.
        // Darker than the backdrop, so the picture reads as the picture.
        constexpr antwika::gfx::Color kSurround{
            .red = 8, .green = 8, .blue = 10, .alpha = 255};
    } // namespace

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

        // The console over the editor, so the sheet stands on top.
        // An empty list while no console is mounted paints nothing.
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

} // namespace antwika::music_editor
