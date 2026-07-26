#pragma once

#include <gmock/gmock.h>

#include "antwika/engine/IEngine.hpp"

namespace antwika::engine::mocks
{

    class MockEngine : public antwika::engine::IEngine
    {
    public:
        MOCK_METHOD(void, start, (), (override));
    };

} // namespace antwika::engine::mocks
