#pragma once

#include <gmock/gmock.h>

#include "antwika/engine/IEngine.hpp"

using antwika::engine::IEngine;

namespace antwika::engine::mocks
{

    class MockEngine : public IEngine
    {
    public:
        MOCK_METHOD(void, start, (), (override));
    };

} // namespace antwika::engine::mocks
