#include <gtest/gtest.h>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/console/ConsoleState.hpp"

using antwika::console::ConsoleState;
using antwika::console::consoleHeightAt;
using antwika::console::kConsoleAnimTicks;
using antwika::gfx::Point;

namespace
{
    constexpr antwika::gfx::Size kTestCanvas{
        .width = 1024, .height = 640};

    constexpr auto kCanvas = kTestCanvas;

    void openFully(ConsoleState &console)
    {
        console.toggle();

        for (std::uint32_t step = 0; step < kConsoleAnimTicks; ++step)
        {
            console.advance();
        }
    }
} // namespace

TEST(ConsoleStateTest, Advance_SlidesInOverTheAnimationTicks)
{
    ConsoleState console;

    EXPECT_FALSE(console.visible());
    EXPECT_FALSE(console.acceptsText());

    console.toggle();

    // The toggle alone shows nothing; the first tick does.
    EXPECT_FALSE(console.visible());

    console.advance();

    EXPECT_TRUE(console.visible());
    EXPECT_FALSE(console.acceptsText());

    for (std::uint32_t step = 1; step < kConsoleAnimTicks; ++step)
    {
        console.advance();
    }

    EXPECT_TRUE(console.acceptsText());
    EXPECT_EQ(console.steps(), kConsoleAnimTicks);

    // Fully open, another tick holds rather than overshoots.
    console.advance();
    EXPECT_EQ(console.steps(), kConsoleAnimTicks);
}

TEST(ConsoleStateTest, Toggle_MidSlideTurnsBackFromWhereItIs)
{
    ConsoleState console;
    console.toggle();
    console.advance();
    console.advance();

    console.toggle();

    // Two steps out means two steps back, not a snap.
    EXPECT_FALSE(console.acceptsText());
    console.advance();
    EXPECT_TRUE(console.visible());
    console.advance();
    EXPECT_FALSE(console.visible());

    // Closed, another tick holds rather than underflows.
    console.advance();
    EXPECT_FALSE(console.visible());
}

TEST(ConsoleStateTest, Toggle_ClosingTakesTheFieldWithIt)
{
    ConsoleState console;
    openFully(console);

    EXPECT_TRUE(console.acceptsText());

    console.toggle();

    // On its way out it no longer reads, even at full height.
    EXPECT_FALSE(console.acceptsText());
}

TEST(ConsoleStateTest, ConsoleHeightAt_AnchorsBothEndsAndOnlyGrows)
{
    EXPECT_EQ(consoleHeightAt(0, kCanvas), 0U);
    EXPECT_EQ(
        consoleHeightAt(kConsoleAnimTicks, kCanvas),
        kCanvas.height / 2);

    // A slide is a slide: every tick stands at least as tall.
    std::uint32_t last = 0;

    for (std::uint32_t step = 1; step <= kConsoleAnimTicks; ++step)
    {
        const auto tall = consoleHeightAt(step, kCanvas);

        EXPECT_GT(tall, last);
        last = tall;
    }
}

TEST(ConsoleStateTest, Covers_AnswersForTheSheetAndOnlyWhileOut)
{
    ConsoleState console;

    EXPECT_FALSE(console.covers(Point{.x = 10, .y = 10}));

    openFully(console);
    console.setHeight(consoleHeightAt(console.steps(), kCanvas));

    const auto bottom =
        static_cast<std::int32_t>(kCanvas.height / 2);

    EXPECT_TRUE(console.covers(Point{.x = 10, .y = 10}));
    EXPECT_TRUE(console.covers(Point{.x = 10, .y = bottom - 1}));
    EXPECT_FALSE(console.covers(Point{.x = 10, .y = bottom}));
}

TEST(ConsoleStateTest, SetLine_TakeLine_HoldAndClearTheField)
{
    ConsoleState console;

    EXPECT_EQ(console.line(), "");

    console.setLine("dump_state", 4);

    EXPECT_EQ(console.line(), "dump_state");
    EXPECT_EQ(console.caret(), 4U);

    EXPECT_EQ(console.takeLine(), "dump_state");
    EXPECT_EQ(console.line(), "");
    EXPECT_EQ(console.caret(), antwika::ui::kCaretAtEnd);
}

TEST(ConsoleStateTest, History_AppendsAndIsReplacedWholesale)
{
    ConsoleState console;

    console.pushHistory("> help");
    console.pushHistory("unknown command: help");

    EXPECT_EQ(
        console.history(),
        (std::vector<std::string>{"> help", "unknown command: help"}));

    console.replaceHistory({"> dump_state"});

    EXPECT_EQ(
        console.history(),
        (std::vector<std::string>{"> dump_state"}));
}
