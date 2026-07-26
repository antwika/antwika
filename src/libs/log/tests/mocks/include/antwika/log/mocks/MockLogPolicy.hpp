#pragma once

#include <gmock/gmock.h>

#include <antwika/log/ILogPolicy.hpp>
#include <antwika/log/Level.hpp>

namespace antwika::log::mocks
{

    using antwika::log::ILogPolicy;
    using antwika::log::Level;

    class MockLogPolicy : public ILogPolicy
    {
    public:
        MOCK_METHOD(bool, accepts, (Level level), (const, noexcept, override));
    };

} // namespace antwika::log::mocks
