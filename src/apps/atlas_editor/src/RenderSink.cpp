#include "antwika/atlas_editor/RenderSink.hpp"

#include <antwika/engine/Events.hpp>
#include <antwika/ui/Painter.hpp>

#include "antwika/atlas_editor/SceneSnapshot.hpp"

namespace antwika::atlas_editor
{

    RenderSink::RenderSink(
        IWindow &window,
        const EditorScene &scene,
        const EditorState &state,
        const UiOverlay &overlay,
        ISleeper &sleeper,
        const std::chrono::milliseconds framePeriod)
        : window(window),
          scene(scene),
          state(state),
          overlay(overlay),
          sleeper(sleeper),
          framePeriod(framePeriod)
    {
    }

    void RenderSink::uploadIfChanged()
    {
        // An optional rather than a count starting at zero.
        // A canvas nobody has edited is at revision zero too.
        // A run would then draw its first frame with no sheet in it.
        if (uploaded == state.image().revision())
        {
            return;
        }

        sheet = window.renderer().createTexture(state.image().bitmap());
        uploaded = state.image().revision();
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

        uploadIfChanged();

        auto &renderer = window.renderer();
        scene.draw(renderer, snapshotOf(state), sheet.get());

        // The bar goes on last, so it reads as being in front.
        // The sheet under it is covered rather than clipped.
        // antwika::gfx has no scissor, and paint order is the depth.
        antwika::ui::paint(renderer, overlay.commands());
        renderer.present();

        sleeper.sleep(framePeriod);
    }

} // namespace antwika::atlas_editor
