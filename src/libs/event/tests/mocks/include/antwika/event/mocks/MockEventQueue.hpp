#pragma once

#include <gmock/gmock.h>

#include <antwika/event/IEventQueue.hpp>

namespace antwika::event::mocks
{

    /**
     * @brief GMock double for IEventQueue.
     */
    class MockEventQueue : public IEventQueue
    {
    public:
        MOCK_METHOD(void, enqueue, (Event event), (override));
        MOCK_METHOD(Event, pop, (), (override));
        MOCK_METHOD(bool, empty, (), (const, noexcept, override));
    };

} // namespace antwika::event::mocks
