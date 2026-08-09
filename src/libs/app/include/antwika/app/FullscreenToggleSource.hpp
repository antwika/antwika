#pragma once

#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/input/IInputEventCodec.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/event/ITickEventSource.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::app
{

    using antwika::event::Event;
    using antwika::gfx::IWindow;
    using antwika::input::IInputEventCodec;
    using antwika::input::Key;
    using antwika::event::ITickEventSource;

    class FullscreenToggleSource final : public ITickEventSource
    {
    public:
        FullscreenToggleSource(
            ITickEventSource &inner,
            IWindow &window,
            const IInputEventCodec &codec,
            Key key);

        FullscreenToggleSource(const FullscreenToggleSource &) = delete;
        FullscreenToggleSource(FullscreenToggleSource &&) = delete;

        FullscreenToggleSource &operator=(
            const FullscreenToggleSource &) = delete;
        FullscreenToggleSource &operator=(
            FullscreenToggleSource &&) = delete;

        [[nodiscard]] std::vector<Event> eventsFor(
            antwika::time::Tick tick) override;

    private:
        ITickEventSource &inner;
        IWindow &window;
        const IInputEventCodec &codec;
        Key key;
    };

}
