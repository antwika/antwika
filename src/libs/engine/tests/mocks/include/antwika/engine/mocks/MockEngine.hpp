#pragma once

#include <gmock/gmock.h>

#include "antwika/engine/IEngine.hpp"

namespace antwika::engine::mocks
{

    using antwika::engine::IEngine;

    /**
     * @brief GMock double for IEngine.
     */
    class MockEngine : public IEngine
    {
    public:
        MOCK_METHOD(void, start, (), (override));
        MOCK_METHOD(void, step, (antwika::time::Tick tick), (override));
    };

} // namespace antwika::engine::mocks
