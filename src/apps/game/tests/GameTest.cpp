#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/engine/mocks/MockEngine.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/game/Game.hpp"

using antwika::engine::mocks::MockEngine;
using antwika::game::Game;
using antwika::log::Level;
using antwika::log::mocks::MockLogger;

// The announcement is a log line now, so there is no dispatch to fail.
// ILogger::log is noexcept, which is what took the old second test away.
TEST(GameTest, Run_AnnouncesTheRunThenStartsTheEngine)
{
    MockEngine mockEngine;
    MockLogger mockLogger;
    Game game(mockEngine, mockLogger);

    {
        ::testing::InSequence seq;
        EXPECT_CALL(
            mockLogger, log(Level::Info, ::testing::HasSubstr("Game")))
            .Times(1);
        EXPECT_CALL(mockEngine, start()).Times(1);
    }

    game.run();
}
