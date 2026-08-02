#include "antwika/music_editor/PasteSource.hpp"

#include <string>
#include <variant>

#include <antwika/input/InputEvent.hpp>
#include <antwika/input/Key.hpp>

#include "antwika/music_editor/Events.hpp"

namespace antwika::music_editor
{

    PasteSource::PasteSource(
        ITickEventSource &inner,
        const IClipboard &clipboard,
        const IInputEventCodec &codec,
        const bool readsClipboard)
        : inner(inner),
          clipboard(clipboard),
          codec(codec),
          readsClipboard(readsClipboard)
    {
    }

    std::vector<Event> PasteSource::eventsFor(const time::Tick tick)
    {
        auto handed = inner.eventsFor(tick);

        // A replay's pastes are already in the file.
        // Reading a clipboard as well would paste twice.
        if (!readsClipboard)
        {
            return handed;
        }

        // Indexed, because a paste appends while this walks.
        // What is appended is behind the size read here.
        const auto arrived = handed.size();

        for (std::size_t at = 0; at < arrived; ++at)
        {
            const auto decoded = codec.decode(handed[at]);

            if (!decoded.has_value())
            {
                continue;
            }

            const auto *pressed =
                std::get_if<antwika::input::KeyPressed>(&*decoded);

            if (pressed == nullptr
                || pressed->key != antwika::input::Key::V
                || !pressed->modifiers.control || pressed->repeat)
            {
                continue;
            }

            const auto text = clipboard.text();

            // An empty clipboard pastes nothing.
            // Saying so would be an event with nothing to say.
            if (text.empty())
            {
                continue;
            }

            // The excluded line's unexercised edges are storage's.
            // The name is a literal too short to reach a heap.
            // The payload's copy throws only on a failed allocation.
            // So does the vector's growth.
            // See docs/confirming-unreachable-branches.md.
            handed.push_back(Event{ // GCOVR_EXCL_LINE
                .name = events::kPaste, // GCOVR_EXCL_LINE
                .payload = text}); // GCOVR_EXCL_LINE
        }

        return handed;
        // The excluded brace is the locals' unwind destructor.
    } // GCOVR_EXCL_LINE

} // namespace antwika::music_editor
