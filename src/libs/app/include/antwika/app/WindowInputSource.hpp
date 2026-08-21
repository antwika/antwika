#pragma once

#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/event/ITickEventSource.hpp>
#include <antwika/gfx/IGfxBackend.hpp>
#include <antwika/gfx/WindowId.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::app
{

    using antwika::event::Event;
    using antwika::event::ITickEventSource;
    using antwika::gfx::IGfxBackend;
    using antwika::gfx::WindowId;

    class WindowInputSource final : public ITickEventSource
    {
    public:
        WindowInputSource(
            ITickEventSource &innerSource,
            IGfxBackend &backend,
            WindowId window);

        WindowInputSource(const WindowInputSource &) = delete;
        WindowInputSource(WindowInputSource &&) = delete;

        WindowInputSource &operator=(const WindowInputSource &) = delete;
        WindowInputSource &operator=(WindowInputSource &&) = delete;

        [[nodiscard]] std::vector<Event> eventsFor(
            antwika::time::Tick tick) override;

    private:
        ITickEventSource &inner;
        IGfxBackend &backend;
        WindowId window;
    };

}
