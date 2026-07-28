#pragma once

#include <gmock/gmock.h>

#include <string_view>

#include <antwika/log/IFormatter.hpp>
#include <antwika/log/Level.hpp>

namespace antwika::log::mocks
{

    using antwika::log::IFormatter;
    using antwika::log::Level;

    /**
     * @brief GMock double for IFormatter.
     */
    class MockFormatter : public IFormatter
    {
    public:
        MOCK_METHOD(std::string, format, (std::chrono::system_clock::time_point time, Level level, std::string_view message), (const, override));
    };

} // namespace antwika::log::mocks
