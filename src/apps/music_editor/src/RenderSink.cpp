#include "antwika/music_editor/RenderSink.hpp"

#include <antwika/engine/Events.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/ViewportRenderer.hpp>
#include <antwika/ui/Painter.hpp>

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
        if (event.event.name != antwika::engine::events::kTick)
        {
            return;
        }

        if (!window.isOpen())
        {
            return;
        }

        // A new one each frame, so fullscreen needs no code here.
        // The next frame simply reads the size the toggle left.
        antwika::gfx::ViewportRenderer view(
            window.renderer(), window.size(), canvas);

        scene.draw(view, editor.commands());

        // The console over the editor, so the sheet stands on top.
        // An empty list while no console is mounted paints nothing.
        antwika::ui::paint(view, console.commands());

        // Last, so whatever reached past the canvas is covered.
        // Nothing at all when the window is the canvas's own shape.
        view.fillSurround(kSurround);
        view.present();
    }

} // namespace antwika::music_editor
