#pragma once

#include <gmock/gmock.h>

#include <string_view>

#include <antwika/log/IFormatter.hpp>
#include <antwika/log/Level.hpp>

using antwika::log::IFormatter;
using antwika::log::Level;

namespace antwika::log::mocks
{

    class MockFormatter : public IFormatter
    {
    public:
        MOCK_METHOD(std::string, format, (std::chrono::system_clock::time_point time, Level level, std::string_view message), (override));
    };

} // namespace antwika::log::mocks
