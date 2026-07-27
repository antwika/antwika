#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/engine/mocks/MockEngine.hpp>
#include <antwika/event/mocks/MockEventDispatcher.hpp>

#include "antwika/game/Game.hpp"

using antwika::engine::mocks::MockEngine;
using antwika::event::Event;
using antwika::event::mocks::MockEventDispatcher;
using antwika::game::Game;

TEST(GameTest, Run_DispatchesBootEventAndStartsEngine)
{
    MockEngine mockEngine;
    MockEventDispatcher mockEventDispatcher;
    Game game(mockEngine, mockEventDispatcher);

    {
        ::testing::InSequence seq;
        EXPECT_CALL(mockEventDispatcher, dispatch(Event{.name = "Running Antwika Game"})).Times(1);
        EXPECT_CALL(mockEngine, start()).Times(1);
    }

    game.run();
}
