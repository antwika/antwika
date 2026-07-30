#pragma once

#include <gmock/gmock.h>

#include <antwika/holdem/Action.hpp>
#include <antwika/holdem/IAgent.hpp>
#include <antwika/holdem/TableView.hpp>

namespace antwika::holdem::mocks
{

    using antwika::holdem::IAgent;

    /**
     * @brief GMock double for IAgent.
     */
    class MockAgent : public IAgent
    {
    public:
        MOCK_METHOD(Action, act, (const TableView &view), (override));
    };

} // namespace antwika::holdem::mocks
