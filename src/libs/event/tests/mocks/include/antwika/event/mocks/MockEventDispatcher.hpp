#pragma once

#include <gmock/gmock.h>

#include <antwika/event/IEventDispatcher.hpp>

namespace antwika::event::mocks
{

    class MockEventDispatcher : public IEventDispatcher
    {
    public:
        MOCK_METHOD(void, dispatch, (const Event &event), (override));
    };

} // namespace antwika::event::mocks
