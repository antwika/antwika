#pragma once

#include <gmock/gmock.h>
#include <antwika/log/Logger.hpp>

#include <string_view>

namespace antwika::log::mocks {

class MockLogger: public antwika::log::ILogger {
public:
    MOCK_METHOD(void, trace, (std::string_view message), (override));
    MOCK_METHOD(void, debug, (std::string_view message), (override));
    MOCK_METHOD(void, info, (std::string_view message), (override));
    MOCK_METHOD(void, warning, (std::string_view message), (override));
    MOCK_METHOD(void, error, (std::string_view message), (override));
    MOCK_METHOD(void, fatal, (std::string_view message), (override));
};

} // namespace antwika::log::mocks
