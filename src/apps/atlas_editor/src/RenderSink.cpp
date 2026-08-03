#include "antwika/atlas_editor/RenderSink.hpp"

#include <antwika/engine/Events.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/ViewportRenderer.hpp>
#include <antwika/ui/Painter.hpp>

#include "antwika/atlas_editor/SceneSnapshot.hpp"

namespace antwika::atlas_editor
{

    namespace
    {
        // What is left over when the window is not the canvas's shape.
        // Painted after the picture rather than before it.
        // Black rather than the scene's own backdrop, deliberately.
        // A letterbox that matched the backdrop would read as sheet.
        constexpr antwika::gfx::Color kSurround{
            .red = 0, .green = 0, .blue = 0};
    } // namespace

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
        // The loads are half the key, and not decoration.
        // replace() installs a new canvas, which begins at revision 0.
        // A load with nothing painted yet moves the revision 0 to 0.
        // The picture would then go on showing the sheet that is gone.
        const UploadKey key{
            .revision = state.image().revision(),
            .loads = state.loads()};

        // An optional rather than a key starting at zero.
        // A canvas nobody has edited or loaded is that key too.
        // A run would then draw its first frame with no sheet in it.
        if (uploaded == key)
        {
            return;
        }

        sheet = window.renderer().createTexture(state.image().bitmap());
        uploaded = key;
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

        // **The one place here reading the size a window reports.**
        // And it reads it to place a picture and nothing else.
        // Every call below is in canvas pixels, exactly as before.
        // This scales and centres them, after every decision is made.
        // A new one each frame, so a resize needs no handling of its own.
        // See docs/resizable-windows.md.
        antwika::gfx::ViewportRenderer view(
            window.renderer(), window.size(), state.canvas());

        scene.draw(view, snapshotOf(state), sheet.get());

        // The bar goes on last, so it reads as being in front.
        // The sheet under it is covered rather than clipped.
        // antwika::gfx has no scissor, and paint order is the depth.
        antwika::ui::paint(view, overlay.commands());

        // Last, so whatever reached past the canvas is covered.
        // Nothing at all when the window is the canvas's own shape.
        view.fillSurround(kSurround);
        view.present();

        sleeper.sleep(framePeriod);
    }

} // namespace antwika::atlas_editor
