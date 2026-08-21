#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/log/Level.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/input/InputCapabilities.hpp"
#include "antwika/input/NullInputBackend.hpp"

using antwika::input::InputCapabilities;
using antwika::input::NullInputBackend;
using antwika::log::Level;
using antwika::log::mocks::MockLogger;
using ::testing::NiceMock;

TEST(NullInputBackendTest, Name_IsNull)
{
    NiceMock<MockLogger> logger;
    NullInputBackend backend(logger);

    EXPECT_EQ(backend.name(), "null");
}

TEST(NullInputBackendTest, Ctor_LogsThatItReportsNothing)
{
    MockLogger logger;

    EXPECT_CALL(logger, log(Level::Debug, "input.null: reporting no input"));

    NullInputBackend backend(logger);
}

TEST(NullInputBackendTest, Capabilities_ClaimsBothDevices)
{
    NiceMock<MockLogger> logger;
    NullInputBackend backend(logger);

    EXPECT_EQ(
        backend.capabilities(),
        (InputCapabilities{.keyboard = true, .pointer = true}));
}

TEST(NullInputBackendTest, PollEvent_AlwaysReturnsNullopt)
{
    NiceMock<MockLogger> logger;
    NullInputBackend backend(logger);

    EXPECT_FALSE(backend.pollEvent().has_value());
    EXPECT_FALSE(backend.pollEvent().has_value());
}
