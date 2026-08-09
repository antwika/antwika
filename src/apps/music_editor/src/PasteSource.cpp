#include "antwika/music_editor/PasteSource.hpp"

#include <string>
#include <variant>

#include <antwika/event/ITickEventSource.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/Key.hpp>

#include "antwika/music_editor/Events.hpp"
#include "antwika/music_editor/PasteText.hpp"

namespace antwika::music_editor
{

    using antwika::event::ITickEventSource;

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

        if (!readsClipboard)
        {
            return handed;
        }

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

            const auto text = pasteableTextOf(clipboard.text());

            if (text.empty())
            {
                continue;
            }

            handed.push_back(Event{ // GCOVR_EXCL_LINE
                .name = events::kPaste, // GCOVR_EXCL_LINE
                .payload = text}); // GCOVR_EXCL_LINE
        }

        return handed;
    } // GCOVR_EXCL_LINE

}
