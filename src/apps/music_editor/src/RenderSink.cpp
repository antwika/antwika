#include "antwika/music_editor/RenderSink.hpp"

#include <antwika/engine/Events.hpp>

namespace antwika::music_editor
{

    RenderSink::RenderSink(
        IWindow &window,
        const EditorScene &scene,
        const EditorSink &editor)
        : window(window), scene(scene), editor(editor)
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

        auto &renderer = window.renderer();

        scene.draw(renderer, editor.commands());
        renderer.present();
    }

} // namespace antwika::music_editor
