#pragma once

#include <gmock/gmock.h>
#include <antwika/log/IFormatter.hpp>
#include <antwika/log/Level.hpp>

#include <string_view>

namespace antwika::log::mocks
{

    class MockFormatter : public antwika::log::IFormatter
    {
    public:
        MOCK_METHOD(std::string, format, (std::chrono::system_clock::time_point time, antwika::log::Level level, std::string_view message), (override));
    };

} // namespace antwika::log::mocks
