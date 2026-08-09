#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>

#include <antwika/console/ConsolePicture.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/time/ISleeper.hpp>

#include "antwika/atlas_editor/EditorScene.hpp"
#include "antwika/atlas_editor/EditorState.hpp"
#include "antwika/atlas_editor/UiOverlay.hpp"

namespace antwika::atlas_editor
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::gfx::ITexture;
    using antwika::gfx::IWindow;
    using antwika::time::ISleeper;

    class RenderSink final : public ITickEventSink
    {
    public:
        RenderSink(
            IWindow &window,
            const EditorScene &scene,
            const EditorState &state,
            const UiOverlay &overlay,
            const antwika::console::ConsolePicture &console,
            ISleeper &sleeper,
            std::chrono::milliseconds framePeriod);

        RenderSink(const RenderSink &) = delete;
        RenderSink(RenderSink &&) = delete;

        RenderSink &operator=(const RenderSink &) = delete;
        RenderSink &operator=(RenderSink &&) = delete;

        void handle(const TickEvent &event) override;

    private:
        struct UploadKey final
        {
            std::uint64_t revision = 0;
            std::uint32_t loads = 0;

            [[nodiscard]] bool operator==(const UploadKey &other) const
                = default;
        };

        void uploadIfChanged();

        IWindow &window;
        const EditorScene &scene;
        const EditorState &state;
        const UiOverlay &overlay;
        const antwika::console::ConsolePicture &console;
        ISleeper &sleeper;
        std::chrono::milliseconds framePeriod;

        std::unique_ptr<ITexture> sheet;
        std::optional<UploadKey> uploaded;
    };

}
