#pragma once

#include <vector>

#include "antwika/event/ITickEventSink.hpp"
#include "antwika/event/TickEvent.hpp"

namespace antwika::event
{

    class TickEventRecorder final : public ITickEventSink
    {
    public:
        void handle(const TickEvent &event) override;

        [[nodiscard]] const std::vector<TickEvent> &getEvents() const;

    private:
        std::vector<TickEvent> events;
    };

}
