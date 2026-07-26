#pragma once

#include <gmock/gmock.h>

#include "antwika/log/ILogPolicy.hpp"

namespace antwika::log::mocks
{

    class MockLogPolicy : public antwika::log::ILogPolicy
    {
    public:
        MOCK_METHOD(bool, accepts, (Level level), (const, noexcept, override));
    };

} // namespace antwika::log::mocks
