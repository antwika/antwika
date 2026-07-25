#pragma once

#include <gmock/gmock.h>

#include "antwika/event/IEventRecorder.hpp"
#include "antwika/event/IEventQueue.hpp"

namespace antwika::event::mocks
{

    class MockEventQueue : public antwika::event::IEventQueue
    {
    public:
        MOCK_METHOD(void, enqueue, (Event event), ());
        MOCK_METHOD(Event, pop, (), ());
        MOCK_METHOD(bool, empty, (), (const, noexcept));
        MOCK_METHOD(std::vector<Event>, getHistory, (), (const));
    };

} // namespace antwika::event::mocks
