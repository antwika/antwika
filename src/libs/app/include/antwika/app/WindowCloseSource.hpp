#pragma once

#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/gfx/IGfxBackend.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/event/ITickEventSource.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::app
{

    using antwika::event::Event;
    using antwika::gfx::IGfxBackend;
    using antwika::gfx::IWindow;
    using antwika::event::ITickEventSource;

    class WindowCloseSource final : public ITickEventSource
    {
    public:
        WindowCloseSource(
            ITickEventSource &innerSource,
            IGfxBackend &backend,
            IWindow &window);

        WindowCloseSource(const WindowCloseSource &) = delete;
        WindowCloseSource(WindowCloseSource &&) = delete;

        WindowCloseSource &operator=(const WindowCloseSource &) = delete;
        WindowCloseSource &operator=(WindowCloseSource &&) = delete;

        [[nodiscard]] std::vector<Event> eventsFor(
            antwika::time::Tick tick) override;

        void pollEvents();

    private:
        ITickEventSource &inner;
        IGfxBackend &backend;
        IWindow &window;
    };

}
