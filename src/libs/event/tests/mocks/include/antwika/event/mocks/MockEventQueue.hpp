#pragma once

#include <gmock/gmock.h>

#include <antwika/event/IEventQueue.hpp>

namespace antwika::event::mocks
{

    class MockEventQueue : public IEventQueue
    {
    public:
        MOCK_METHOD(void, enqueue, (Event event), ());
        MOCK_METHOD(Event, pop, (), ());
        MOCK_METHOD(bool, empty, (), (const, noexcept));
    };

} // namespace antwika::event::mocks
