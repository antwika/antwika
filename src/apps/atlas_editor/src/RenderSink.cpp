#include "antwika/atlas_editor/RenderSink.hpp"

#include <antwika/app/FramePresentation.hpp>
#include <antwika/gfx/Color.hpp>

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
        const antwika::console::ConsolePicture &console,
        ISleeper &sleeper,
        const std::chrono::milliseconds framePeriod)
        : window(window),
          scene(scene),
          state(state),
          overlay(overlay),
          console(console),
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
        if (!antwika::app::drawsOn(event, window))
        {
            return;
        }

        uploadIfChanged();

        // The console goes over the bar, being over everything.
        // Empty and free whenever no console is mounted or it is in.
        antwika::app::presentViewport(
            window,
            state.canvas(),
            kSurround,
            console,
            [this](antwika::gfx::IRenderer &view)
            {
                scene.draw(view, snapshotOf(state), sheet.get());

                // The bar goes on last, so it reads as being in front.
                // The sheet under it is covered rather than clipped.
                // antwika::gfx has no scissor, and paint order is depth.
                antwika::app::paintOver(view, overlay);
            });

        sleeper.sleep(framePeriod);
    }

} // namespace antwika::atlas_editor
