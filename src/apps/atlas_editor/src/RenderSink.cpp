#include "antwika/atlas_editor/RenderSink.hpp"

#include <antwika/app/FramePresentation.hpp>
#include <antwika/gfx/Color.hpp>

#include "antwika/atlas_editor/SceneSnapshot.hpp"

namespace antwika::atlas_editor
{

    namespace
    {
        constexpr antwika::gfx::Color kSurround{
            .red = 0, .green = 0, .blue = 0};
    }

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
        const UploadKey key{
            .revision = state.image().revision(),
            .loads = state.loads()};

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

        antwika::app::presentViewport(
            window,
            state.canvas(),
            kSurround,
            console,
            [this](antwika::gfx::IRenderer &view)
            {
                scene.draw(view, snapshotOf(state), sheet.get());

                antwika::app::paintOver(view, overlay);
            });

        sleeper.sleep(framePeriod);
    }

}
