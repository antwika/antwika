#pragma once

#include <gmock/gmock.h>

#include <antwika/event/IEventDispatcher.hpp>

namespace antwika::event::mocks
{

    class MockEventDispatcher final : public IEventDispatcher
    {
    public:
        MOCK_METHOD(void, dispatch, (Event event), (override));
    };

}
