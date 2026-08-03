#pragma once

#include <cstdint>
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
     *
     * **Every event describes the editor, acts on what came back, and
     * describes it again**, which is the remedy antwika::ui's own
     * Context::finish() gives: a press is resolved while the frame is
     * being laid out, so the picture beside it predates whatever the
     * press changed. Two descriptions cost one more layout and no
     * retained state, and only the first one's answers are read -- or
     * a press would activate twice.
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
         * @param clipboard Where a copy is mirrored to, or null.
         * **Null on a replay**, so replaying somebody's session does
         * not overwrite this machine's clipboard with their copies;
         * the mirror is an outward write read back by nothing, on the
         * same terms a frame is drawn.  Pastes never come from here --
         * they arrive as events::kPaste, read upstream by PasteSource.
         * @param stop Told when the menu's quit is chosen, exactly as
         * apps/game's main menu tells its loop: the recording holds
         * the click, and the stop follows from it on every replay.
         * Must outlive this object.
         * @param scoresDirectory Where the save box writes and the
         * load box reads, which is the one place this sink names a
         * file.  **Loading reads a file inside the tick path**, on the
         * terms apps/game::SaveLoadSink documents: the list is fixed
         * for the run, but a score's *contents* can only be read when
         * the click asking for them arrives, so a replay reproduces a
         * load exactly as long as the file still holds what it held.
         * @param writesScores Whether a save reaches the disk.
         * **False on a replay**, so replaying somebody's session does
         * not overwrite this machine's scores with theirs -- while
         * everything the state does on a save happens identically, or
         * the replay would diverge from the run at that click.
         * The one divergence left is a live save that *failed*: the
         * disk's answer is machine state no recording carries, so a
         * replay reproduces the save that succeeded.
         */
        EditorSink(
            EditorState &state,
            Score &score,
            Playback &playback,
            const IInputEventCodec &codec,
            const EditorScene &scene,
            Size canvas,
            input::IClipboard *clipboard,
            ITickEventSink &stop,
            std::string scoresDirectory,
            bool writesScores);

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
        /**
         * @brief What one event's pointer did, beyond where it is.
         *
         * Two edges rather than a folded state, because both are
         * properties of the event being handled and neither survives
         * it. See ui::Pointer::extends.
         */
        struct PointerEdge
        {
            /** @brief Whether a button went down for this event. */
            bool pressed = false;

            /**
             * @brief Whether this carries a selection on rather than
             * starting one: shift held, or a drag under way.
             */
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
        input::IClipboard *clipboard;
        ITickEventSink &stop;
        std::string scoresDirectory;
        bool writesScores;

        // What the mirror last wrote, so it writes on changes alone.
        std::string mirrored;

        antwika::input::InputState folded;
        time::Tick foldedTick = 0;
        bool located = false;

        ui::DrawList picture;
    };

} // namespace antwika::music_editor
