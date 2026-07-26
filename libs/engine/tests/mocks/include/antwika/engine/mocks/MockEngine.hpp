#pragma once

#include <gmock/gmock.h>

#include "antwika/engine/Engine.hpp"

namespace antwika::engine::mocks
{

    class MockEngine : public antwika::engine::Engine
    {
    public:
        MOCK_METHOD(void, start, (), (override));
    };

} // namespace antwika::engine::mocks
