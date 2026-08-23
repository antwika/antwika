#pragma once

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>

namespace antwika::engine
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;

    class StopSignal final : public ITickEventSink
    {
    public:
        void handle(const TickEvent &event) override;

        [[nodiscard]] bool isStopped() const noexcept;

    private:
        bool stopped = false;
    };

}
