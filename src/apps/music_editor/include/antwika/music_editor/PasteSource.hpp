#pragma once

#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/input/IClipboard.hpp>
#include <antwika/input/IInputEventCodec.hpp>
#include <antwika/simulation/ITickEventSource.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::music_editor
{

    using antwika::event::Event;
    using antwika::input::IClipboard;
    using antwika::input::IInputEventCodec;
    using antwika::simulation::ITickEventSource;

    /**
     * @brief Turns Ctrl+V into the clipboard's characters, as an event.
     *
     * **The clipboard is read here, above the loop and upstream of the
     * recorder, because what it holds is external input.** A sink
     * reading it would read the replaying machine's clipboard on a
     * replay and diverge; this reads it where a key press is read, so
     * the recording carries the characters and a replay pastes what
     * the run pasted, whatever the replaying machine holds.
     *
     * The key press itself passes through untouched and is recorded
     * like any other, exactly as app::FullscreenToggleSource leaves
     * its key in the stream.  What is added is one events::kPaste
     * event after it, carrying the text -- nothing when the clipboard
     * is empty, and nothing for a held key's repeats, or one long
     * press would flood a recording with copies of the payload.
     *
     * **A replay run must not read a clipboard at all**, so the same
     * flag input::InputPipeline takes decides: constructed with
     * readsClipboard false this is a pure pass-through, and the pastes
     * arrive from the file like everything else.
     */
    class PasteSource final : public ITickEventSource
    {
    public:
        /**
         * @brief Construct the source over what it decorates.
         * @param inner Supplies each tick's events; must outlive this.
         * @param clipboard What Ctrl+V reads; must outlive this.
         * @param codec Decodes the key edges; must outlive this.
         * @param readsClipboard False on a replay, whose pastes are in
         * the file already.
         */
        PasteSource(
            ITickEventSource &inner,
            const IClipboard &clipboard,
            const IInputEventCodec &codec,
            bool readsClipboard);

        PasteSource(const PasteSource &) = delete;
        PasteSource(PasteSource &&) = delete;

        PasteSource &operator=(const PasteSource &) = delete;
        PasteSource &operator=(PasteSource &&) = delete;

        /**
         * @brief Get a tick's events, with a paste after each Ctrl+V.
         * @param tick The tick to fetch events for.
         * @return The inner source's events, in order, with one
         * events::kPaste appended per fresh Ctrl+V press.
         */
        [[nodiscard]] std::vector<Event> eventsFor(
            time::Tick tick) override;

    private:
        ITickEventSource &inner;
        const IClipboard &clipboard;
        const IInputEventCodec &codec;
        bool readsClipboard;
    };

} // namespace antwika::music_editor
