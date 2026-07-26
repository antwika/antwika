#pragma once

#include <gmock/gmock.h>

#include <antwika/log/ILogPolicy.hpp>
#include <antwika/log/Level.hpp>

using antwika::log::ILogPolicy;
using antwika::log::Level;

namespace antwika::log::mocks
{

    class MockLogPolicy : public ILogPolicy
    {
    public:
        MOCK_METHOD(bool, accepts, (Level level), (const, noexcept, override));
    };

} // namespace antwika::log::mocks
