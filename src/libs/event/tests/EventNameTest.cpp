#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <latch>
#include <string_view>
#include <thread>
#include <vector>

#include "antwika/event/EngineEvents.hpp"
#include "antwika/event/EventName.hpp"
#include "antwika/event/EventNameSeeds.hpp"

using antwika::event::EventName;
using antwika::event::kSeededEventNames;

TEST(EventNameTest, Seeds_PinEveryIdToItsText)
{
    const std::array<std::string_view, 9> expectedTexts{
        "",
        "engine.tick",
        "engine.stop",
        "input.key_down",
        "input.key_up",
        "input.pointer_move",
        "input.pointer_down",
        "input.pointer_up",
        "input.pointer_scroll"};

    ASSERT_EQ(kSeededEventNames.size(), expectedTexts.size());

    for (std::size_t index = 0; index < expectedTexts.size(); ++index)
    {
        EXPECT_EQ(kSeededEventNames[index], expectedTexts[index]);
    }
}

TEST(EventNameTest, GetSeeded_MatchesTheRuntimeInternedName)
{
    EXPECT_EQ(EventName{""}, EventName{});
    EXPECT_EQ(EventName{"engine.tick"}, antwika::event::kTick);
    EXPECT_EQ(EventName{"engine.stop"}, antwika::event::kStop);
    EXPECT_EQ(
        EventName{"input.key_down"},
        EventName::getSeeded("input.key_down"));
    EXPECT_EQ(
        EventName{"input.key_up"},
        EventName::getSeeded("input.key_up"));
    EXPECT_EQ(
        EventName{"input.pointer_move"},
        EventName::getSeeded("input.pointer_move"));
    EXPECT_EQ(
        EventName{"input.pointer_down"},
        EventName::getSeeded("input.pointer_down"));
    EXPECT_EQ(
        EventName{"input.pointer_up"},
        EventName::getSeeded("input.pointer_up"));
    EXPECT_EQ(
        EventName{"input.pointer_scroll"},
        EventName::getSeeded("input.pointer_scroll"));
}

TEST(EventNameTest, GetText_ReturnsTheSeededBytes)
{
    EXPECT_EQ(EventName{}.getText(), "");
    EXPECT_EQ(antwika::event::kTick.getText(), "engine.tick");
    EXPECT_EQ(antwika::event::kStop.getText(), "engine.stop");
    EXPECT_EQ(
        EventName::getSeeded("input.key_down").getText(), "input.key_down");
    EXPECT_EQ(
        EventName::getSeeded("input.pointer_scroll").getText(),
        "input.pointer_scroll");
}

TEST(EventNameTest, Ctor_InternsTheSameTextOnce)
{
    const EventName firstName{"custom.intern_once"};
    const EventName secondName{"custom.intern_once"};

    EXPECT_EQ(firstName, secondName);
    EXPECT_EQ(firstName.getText(), "custom.intern_once");
}

TEST(EventNameTest, Ctor_InternsOneTextAcrossThreads)
{
    constexpr std::size_t kThreadCount = 8;
    std::array<EventName, kThreadCount> names;
    std::latch startLine(kThreadCount);
    std::vector<std::thread> threads;
    threads.reserve(kThreadCount);

    for (std::size_t index = 0; index < kThreadCount; ++index)
    {
        threads.emplace_back(
            [&names, &startLine, index]
            {
                startLine.arrive_and_wait();

                names[index] = EventName{"custom.threaded"};
            });
    }

    for (auto &thread : threads)
    {
        thread.join();
    }

    for (const auto &name : names)
    {
        EXPECT_EQ(name, names[0]);
        EXPECT_EQ(name.getText(), "custom.threaded");
    }
}
