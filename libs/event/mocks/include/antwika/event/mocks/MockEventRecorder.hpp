#pragma once

#include <gmock/gmock.h>

#include "antwika/event/IEventQueue.hpp"
#include "antwika/event/IEventHistory.hpp"

namespace antwika::event::mocks
{
    class MockEventRecorder : public IEventSink,
                              public IEventHistory
    {
    public:
        MOCK_METHOD(void, handle, (const Event &event), (override));
        MOCK_METHOD(std::vector<Event>, getEvents, (), (const, override));
    };

} // namespace antwika::event::mocks
