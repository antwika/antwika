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
}

TEST(ConsoleStateTest, Advance_SlidesInOverTheAnimationTicks)
{
    ConsoleState console;

    EXPECT_FALSE(console.visible());
    EXPECT_FALSE(console.acceptsText());

    console.toggle();

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

    EXPECT_FALSE(console.acceptsText());
    console.advance();
    EXPECT_TRUE(console.visible());
    console.advance();
    EXPECT_FALSE(console.visible());

    console.advance();
    EXPECT_FALSE(console.visible());
}

TEST(ConsoleStateTest, Toggle_ClosingTakesTheFieldWithIt)
{
    ConsoleState console;
    openFully(console);

    EXPECT_TRUE(console.acceptsText());

    console.toggle();

    EXPECT_FALSE(console.acceptsText());
}

TEST(ConsoleStateTest, ConsoleHeightAt_AnchorsBothEndsAndOnlyGrows)
{
    EXPECT_EQ(consoleHeightAt(0, kCanvas), 0U);
    EXPECT_EQ(
        consoleHeightAt(kConsoleAnimTicks, kCanvas),
        kCanvas.height / 2);

    std::uint32_t last = 0;

    for (std::uint32_t step = 1; step <= kConsoleAnimTicks; ++step)
    {
        const auto tall = consoleHeightAt(step, kCanvas);

        EXPECT_GT(tall, last);
        last = tall;
    }
}

TEST(ConsoleStateTest, SetHeight_HoldsTheSheetHeightFromNothing)
{
    ConsoleState console;

    EXPECT_EQ(console.height(), 0U);

    console.setHeight(320);

    EXPECT_EQ(console.height(), 320U);
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

TEST(ConsoleStateTest, Recall_WalksBackThroughTheCommandsItRemembered)
{
    ConsoleState console;

    console.rememberCommand("first");
    console.rememberCommand("second");

    console.recall(true);
    EXPECT_EQ(console.line(), "second");

    console.recall(true);
    EXPECT_EQ(console.line(), "first");

    console.recall(true);
    EXPECT_EQ(console.line(), "first");
}

TEST(ConsoleStateTest, Recall_EmptiesTheFieldPastTheNewestCommand)
{
    ConsoleState console;

    console.rememberCommand("first");
    console.rememberCommand("second");

    console.recall(true);
    console.recall(true);

    console.recall(false);
    EXPECT_EQ(console.line(), "second");

    console.recall(false);
    EXPECT_TRUE(console.line().empty());

    console.recall(false);
    EXPECT_TRUE(console.line().empty());
}

TEST(ConsoleStateTest, Recall_EmptiesTheFieldWithNothingRemembered)
{
    ConsoleState console;

    console.setLine("typed", 0);
    console.recall(true);

    EXPECT_TRUE(console.line().empty());
    EXPECT_TRUE(console.commands().empty());
}

TEST(ConsoleStateTest, RememberCommand_KeepsWhatWasEnteredInOrder)
{
    ConsoleState console;

    console.rememberCommand("first");
    console.rememberCommand("second");

    EXPECT_EQ(
        console.commands(),
        (std::vector<std::string>{"first", "second"}));
}
