#pragma once

#include <gmock/gmock.h>

#include <antwika/event/ITimedEventSink.hpp>

namespace antwika::event::mocks
{

    using antwika::event::ITimedEventSink;

    /**
     * @brief GMock double for ITimedEventSink.
     */
    class MockTimedEventSink : public ITimedEventSink
    {
    public:
        MOCK_METHOD(void, handle, (const TimedEvent &event), (override));
    };

} // namespace antwika::event::mocks
