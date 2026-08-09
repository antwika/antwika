#pragma once

#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/input/IClipboard.hpp>
#include <antwika/input/IInputEventCodec.hpp>
#include <antwika/event/ITickEventSource.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::music_editor
{

    using antwika::event::Event;
    using antwika::input::IClipboard;
    using antwika::input::IInputEventCodec;
    using antwika::event::ITickEventSource;

    class PasteSource final : public ITickEventSource
    {
    public:
        PasteSource(
            ITickEventSource &inner,
            const IClipboard &clipboard,
            const IInputEventCodec &codec,
            bool readsClipboard);

        PasteSource(const PasteSource &) = delete;
        PasteSource(PasteSource &&) = delete;

        PasteSource &operator=(const PasteSource &) = delete;
        PasteSource &operator=(PasteSource &&) = delete;

        [[nodiscard]] std::vector<Event> eventsFor(
            time::Tick tick) override;

    private:
        ITickEventSource &inner;
        const IClipboard &clipboard;
        const IInputEventCodec &codec;
        bool readsClipboard;
    };

}
