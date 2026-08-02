#pragma once

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/IInputEventCodec.hpp>
#include <antwika/input/InputState.hpp>
#include <antwika/time/Tick.hpp>
#include <antwika/ui/DrawList.hpp>
#include <antwika/ui/Keyboard.hpp>
#include <antwika/ui/Pointer.hpp>

#include "antwika/music_editor/EditorScene.hpp"
#include "antwika/music_editor/EditorState.hpp"
#include "antwika/music_editor/Playback.hpp"
#include "antwika/music_editor/Score.hpp"

namespace antwika::music_editor
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::input::IInputEventCodec;

    /**
     * @brief The editor's tick: read the input, change the lines, keep
     * the sound going.
     *
     * **The UI is described and resolved here, inside the tick path and
     * downstream of the recorder.** So a replay stores the keystroke and
     * regenerates which field it went into, and this app defines no
     * event of its own -- there is nothing here that is not worked out
     * again from edges the recording already holds.
     *
     * The order within a tick matters and is deliberate: the lines are
     * re-read from what the input just did to them, and only then is
     * the sound advanced -- so a note decided this tick is decided from
     * the line as it now reads rather than as it read last tick.
     */
    class EditorSink final : public ITickEventSink
    {
    public:
        /**
         * @brief Construct the sink over everything it drives.
         * @param state The lines; must outlive this object.
         * @param score What they parse into; must outlive this.
         * @param playback What sounds them; must outlive this.
         * @param codec Decodes the recorded input events.
         * @param scene Describes the picture; must outlive this.
         * @param canvas The size the window was asked for.
         */
        EditorSink(
            EditorState &state,
            Score &score,
            Playback &playback,
            const IInputEventCodec &codec,
            const EditorScene &scene,
            Size canvas);

        EditorSink(const EditorSink &) = delete;
        EditorSink(EditorSink &&) = delete;

        EditorSink &operator=(const EditorSink &) = delete;
        EditorSink &operator=(EditorSink &&) = delete;

        /**
         * @brief Take one of a tick's events.
         * @param event The event, which may be the tick itself.
         */
        void handle(const TickEvent &event) override;

        /**
         * @brief Get the picture the last description produced.
         * @return The commands, for whatever draws them.
         */
        [[nodiscard]] const ui::DrawList &commands() const noexcept;

    private:
        void refreshAndAct(bool pressed, const ui::Keyboard &keyboard);

        [[nodiscard]] ui::Pointer pointerNow(bool pressed) const;

        EditorState &state;
        Score &score;
        Playback &playback;
        const IInputEventCodec &codec;
        const EditorScene &scene;
        Size canvas;

        antwika::input::InputState folded;
        time::Tick foldedTick = 0;
        bool located = false;

        ui::DrawList picture;
    };

} // namespace antwika::music_editor
