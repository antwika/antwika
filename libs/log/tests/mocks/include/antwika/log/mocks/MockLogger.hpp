#pragma once

#include <gmock/gmock.h>
#include <antwika/log/ILogger.hpp>
#include <antwika/log/Level.hpp>

#include <string_view>

namespace antwika::log::mocks
{

    class MockLogger : public antwika::log::ILogger
    {
    public:
        MOCK_METHOD(void, log, (antwika::log::Level, std::string_view message), (noexcept, override));
    };

} // namespace antwika::log::mocks
