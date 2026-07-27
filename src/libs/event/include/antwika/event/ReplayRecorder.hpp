#pragma once

#include <vector>

#include "ITimedEventHistory.hpp"
#include "ITimedEventSink.hpp"
#include "TimedEvent.hpp"

namespace antwika::event
{

    class ReplayRecorder final : public ITimedEventSink,
                                 public ITimedEventHistory
    {
    public:
        void handle(const TimedEvent &event) override;
        [[nodiscard]] std::vector<TimedEvent> getEvents() const override;

    private:
        std::vector<TimedEvent> events;
    };

} // namespace antwika::event
