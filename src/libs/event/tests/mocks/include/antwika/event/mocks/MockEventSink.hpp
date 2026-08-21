#pragma once

#include <gmock/gmock.h>

#include <antwika/event/IEventSink.hpp>

namespace antwika::event::mocks
{

    using antwika::event::IEventSink;

    class MockEventSink final : public IEventSink
    {
    public:
        MOCK_METHOD(void, handle, (const Event &event), (override));
    };

}
