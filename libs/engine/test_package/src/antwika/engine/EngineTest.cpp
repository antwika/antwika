#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/engine/Engine.hpp"

using antwika::engine::Engine;
using antwika::log::mocks::MockLogger;

TEST(EngineTest, Start_WritesInfoMessageToLogger)
{
    MockLogger mockLogger;
    // antwika::engine::test::MockLogger mockLogger;
    Engine engine(mockLogger);
    EXPECT_CALL(mockLogger, info("Antwika engine started!"));
    engine.start();
}
