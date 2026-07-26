#pragma once

#include <gmock/gmock.h>

#include "antwika/engine/IEngine.hpp"

namespace antwika::engine::mocks
{

    using antwika::engine::IEngine;

    class MockEngine : public IEngine
    {
    public:
        MOCK_METHOD(void, start, (), (override));
    };

} // namespace antwika::engine::mocks
