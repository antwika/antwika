#pragma once

#include <gmock/gmock.h>

#include <string_view>

#include <antwika/log/ILogger.hpp>
#include <antwika/log/Level.hpp>

using antwika::log::ILogger;
using antwika::log::Level;

namespace antwika::log::mocks
{

    class MockLogger : public ILogger
    {
    public:
        MOCK_METHOD(void, log, (Level level, std::string_view message), (noexcept, override));
    };

} // namespace antwika::log::mocks
