#pragma once

#include <gmock/gmock.h>

#include <vector>

#include <antwika/event/ITickEventSource.hpp>

namespace antwika::event::mocks
{

    using antwika::event::ITickEventSource;

    class MockTickEventSource final : public ITickEventSource
    {
    public:
        MOCK_METHOD(
            std::vector<Event>,
            eventsFor,
            (antwika::time::Tick tick),
            (override));
    };

}
