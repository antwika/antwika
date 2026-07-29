#pragma once

#include <gmock/gmock.h>

#include <antwika/event/ITickEventSink.hpp>

namespace antwika::event::mocks
{

    using antwika::event::ITickEventSink;

    /**
     * @brief GMock double for ITickEventSink.
     */
    class MockTickEventSink : public ITickEventSink
    {
    public:
        MOCK_METHOD(void, handle, (const TickEvent &event), (override));
    };

} // namespace antwika::event::mocks
