#pragma once

#include <gmock/gmock.h>

#include <antwika/event/IEventSink.hpp>

using antwika::event::IEventSink;

namespace antwika::event::mocks
{

    class MockEventSink : public IEventSink
    {
    public:
        MOCK_METHOD(void, handle, (const Event &event), (override));
    };

} // namespace antwika::event::mocks
