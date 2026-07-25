#pragma once

#include <gmock/gmock.h>

#include "antwika/event/IEventQueue.hpp"

namespace antwika::event::mocks
{
    class MockEventRecorder : public IEventSink, IEventHistory
    {
    public:
        MOCK_METHOD(void, handle, (const Event &event), ());
        MOCK_METHOD(std::vector<Event>, getEvents, (), (const));
    };

} // namespace antwika::event::mocks
