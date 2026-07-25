#pragma once

#include <gmock/gmock.h>

#include "antwika/event/IEventRecorder.hpp"
#include "antwika/event/IEventQueue.hpp"

namespace antwika::event::mocks
{
    class MockEventRecorder : public antwika::event::IEventRecorder
    {
    public:
        MOCK_METHOD(void, record, (const Event &event), ());
        MOCK_METHOD(std::vector<Event>, getEvents, (), (const));
    };

} // namespace antwika::event::mocks
