#pragma once

#include <gmock/gmock.h>

#include <string_view>

#include <antwika/log/ILogger.hpp>
#include <antwika/log/Level.hpp>

namespace antwika::log::mocks
{

    using antwika::log::ILogger;
    using antwika::log::Level;

    /**
     * @brief GMock double for ILogger.
     */
    class MockLogger : public ILogger
    {
    public:
        MOCK_METHOD(
            void,
            log,
            (Level level, std::string_view message),
            (noexcept, override));
    };

} // namespace antwika::log::mocks
