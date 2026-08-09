#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/IClipboard.hpp>
#include <antwika/input/IInputEventCodec.hpp>
#include <antwika/input/InputState.hpp>
#include <antwika/time/Tick.hpp>
#include <antwika/ui/DrawList.hpp>
#include <antwika/ui/Frame.hpp>
#include <antwika/ui/Keyboard.hpp>
#include <antwika/ui/Pointer.hpp>

#include "antwika/music_editor/EditorScene.hpp"
#include "antwika/music_editor/EditorState.hpp"
#include "antwika/music_editor/Playback.hpp"
#include "antwika/music_editor/Score.hpp"
#include "antwika/music_editor/WaveImageCache.hpp"

namespace antwika::music_editor
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::input::IInputEventCodec;

    class EditorSink final : public ITickEventSink
    {
    public:
        EditorSink(
            EditorState &state,
            Score &score,
            Playback &playback,
            const IInputEventCodec &codec,
            const EditorScene &scene,
            Size canvas,
            WaveRenderDesc waveRender,
            std::optional<std::reference_wrapper<input::IClipboard>>
                clipboard,
            ITickEventSink &stop,
            std::string scoresDirectory,
            bool writesScores);

        EditorSink(const EditorSink &) = delete;
        EditorSink(EditorSink &&) = delete;

        EditorSink &operator=(const EditorSink &) = delete;
        EditorSink &operator=(EditorSink &&) = delete;

        void handle(const TickEvent &event) override;

        [[nodiscard]] const ui::DrawList &commands() const noexcept;

    private:
        struct PointerEdge final
        {
            bool pressed = false;

            bool extends = false;
        };

        void refreshAndAct(PointerEdge edge, const ui::Keyboard &keyboard);

        void modalRefreshAndAct(
            PointerEdge edge, const ui::Keyboard &keyboard);

        void menuAction(std::size_t index);

        void speedAction(std::size_t index);

        void saveNow();

        void loadNow(std::size_t at);

        void mirrorClipboard();

        [[nodiscard]] ui::Frame frameFor(
            PointerEdge edge, const ui::Keyboard &keyboard) const;

        [[nodiscard]] ui::Pointer pointerNow(PointerEdge edge) const;

        void scrollBy(std::int32_t notches);

        EditorState &state;
        Score &score;
        Playback &playback;
        const IInputEventCodec &codec;
        const EditorScene &scene;
        Size canvas;
        std::optional<std::reference_wrapper<input::IClipboard>>
            clipboard;
        ITickEventSink &stop;
        std::string scoresDirectory;
        bool writesScores;

        std::string mirrored;

        WaveRenderDesc waveRender;

        mutable WaveImageCache waveImages;

        antwika::input::InputState folded;
        time::Tick foldedTick = 0;
        bool located = false;

        ui::DrawList picture;
    };

}
