#include <gtest/gtest.h>

#include "antwika/engine/Events.hpp"

using antwika::engine::events::kStop;
using antwika::engine::events::kTick;

TEST(EventsTest, KTick_IsTheNameRecordedReplaysHold)
{
    EXPECT_EQ(kTick.getText(), "engine.tick");
}

TEST(EventsTest, KStop_IsTheNameRecordedReplaysHold)
{
    EXPECT_EQ(kStop.getText(), "engine.stop");
}
