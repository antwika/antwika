#pragma once

#include <gmock/gmock.h>

#include <antwika/reducer/IReducer.hpp>

namespace antwika::reducer::mocks
{

    /**
     * @brief GMock double for IReducer<State>.
     */
    template <typename State>
    class MockReducer : public IReducer<State>
    {
    public:
        MOCK_METHOD(
            State,
            reduce,
            (const State &previous, const TickEvent &event),
            (const, override));
    };

} // namespace antwika::reducer::mocks
