#include <gtest/gtest.h>

#include <string>
#include <variant>
#include <vector>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Glyphs.hpp>
#include <antwika/ui/DrawCommand.hpp>
#include <antwika/ui/Frame.hpp>
#include <antwika/ui/Theme.hpp>
#include <antwika/ui/support/DrawBounds.hpp>

#include "antwika/console/ConsoleScene.hpp"
#include "antwika/console/ConsoleState.hpp"

using antwika::console::ConsoleScene;
using antwika::console::ConsoleState;
using antwika::console::consoleHeightAt;
using antwika::console::kConsoleAnimTicks;
using antwika::console::kConsoleHistoryShown;
using antwika::ui::DrawText;
using antwika::ui::FillRect;
using antwika::ui::scaleForCanvas;
using antwika::ui::support::expectInsideCanvas;

namespace
{
    constexpr antwika::gfx::Size kTestCanvas{
        .width = 1024, .height = 640};

    constexpr auto kCanvas = kTestCanvas;

    void slideTo(ConsoleState &console, std::uint32_t steps)
    {
        console.toggle();

        for (std::uint32_t step = 0; step < steps; ++step)
        {
            console.advance();
        }

        console.setHeight(consoleHeightAt(console.steps(), kCanvas));
    }

    [[nodiscard]] bool saysSomewhere(
        const antwika::ui::Frame &frame, const std::string &line)
    {
        for (const auto &command : frame.commands)
        {
            const auto *text = std::get_if<DrawText>(&command);

            if (text != nullptr && text->text == line)
            {
                return true;
            }
        }

        return false;
    }
}

TEST(ConsoleSceneTest, Describe_ClosedDrawsNothingAtAll)
{
    const ConsoleScene scene;
    const ConsoleState console;

    const auto frame = scene.describe(kCanvas, {}, {}, console);

    EXPECT_TRUE(frame.commands.empty());
    EXPECT_FALSE(
        frame.rects.find(antwika::console::consoleWidgets::kSheet)
            .has_value());
}

TEST(ConsoleSceneTest, Describe_MidSlideIsAnEmptySheetAtItsOwnHeight)
{
    const ConsoleScene scene;
    ConsoleState console;
    slideTo(console, 3);

    const auto frame = scene.describe(kCanvas, {}, {}, console);

    const auto sheet =
        frame.rects.find(antwika::console::consoleWidgets::kSheet);
    ASSERT_TRUE(sheet.has_value());

    EXPECT_EQ(sheet->origin.x, 0);
    EXPECT_EQ(sheet->origin.y, 0);
    EXPECT_EQ(sheet->size.width, kCanvas.width);
    EXPECT_EQ(sheet->size.height, console.height());

    EXPECT_FALSE(
        frame.rects.find(antwika::console::consoleWidgets::kInput)
            .has_value());
}

TEST(ConsoleSceneTest, Describe_OpenPutsTheFieldOnTheSheetsBottomEdge)
{
    const ConsoleScene scene;
    ConsoleState console;
    slideTo(console, kConsoleAnimTicks);

    const auto frame = scene.describe(kCanvas, {}, {}, console);

    const auto sheet =
        frame.rects.find(antwika::console::consoleWidgets::kSheet);
    const auto field =
        frame.rects.find(antwika::console::consoleWidgets::kInput);
    ASSERT_TRUE(sheet.has_value());
    ASSERT_TRUE(field.has_value());

    EXPECT_EQ(sheet->size.height, kCanvas.height / 2);

    const auto fieldBottom = field->origin.y
        + static_cast<std::int32_t>(field->size.height);
    const auto sheetBottom = sheet->origin.y
        + static_cast<std::int32_t>(sheet->size.height);

    EXPECT_EQ(fieldBottom, sheetBottom);
    EXPECT_EQ(field->size.width, kCanvas.width);
}

TEST(ConsoleSceneTest, Describe_FillsTheSheetWithTheConsoleBackdrop)
{
    constexpr antwika::gfx::Color kBackdrop{
        .red = 14, .green = 16, .blue = 24};

    const ConsoleScene scene;
    ConsoleState console;
    slideTo(console, kConsoleAnimTicks);

    const auto frame = scene.describe(kCanvas, {}, {}, console);

    const auto sheet =
        frame.rects.find(antwika::console::consoleWidgets::kSheet);
    ASSERT_TRUE(sheet.has_value());

    std::vector<antwika::gfx::Color> fills;

    for (const auto &command : frame.commands)
    {
        const auto *filled = std::get_if<FillRect>(&command);

        if (filled != nullptr && filled->rect == *sheet)
        {
            fills.push_back(filled->color);
        }
    }

    ASSERT_EQ(fills.size(), 1U);
    EXPECT_EQ(fills.front(), kBackdrop);
}

TEST(ConsoleSceneTest, Describe_WrapsAHistoryLineWiderThanTheCanvas)
{
    constexpr std::size_t kWide = 200;

    const ConsoleScene scene;
    ConsoleState console;
    slideTo(console, kConsoleAnimTicks);

    const std::string wide(kWide, 'x');
    console.pushHistory(wide);

    const auto frame = scene.describe(kCanvas, {}, {}, console);

    EXPECT_FALSE(saysSomewhere(frame, wide));
    expectInsideCanvas(frame.commands, kCanvas);
}

TEST(ConsoleSceneTest, Describe_BreaksAHistoryLineOnItsSpaces)
{
    const ConsoleScene scene;
    ConsoleState console;
    slideTo(console, kConsoleAnimTicks);

    const auto columns = kCanvas.width
        / (antwika::gfx::kGlyphAdvance * scaleForCanvas(kCanvas));

    console.pushHistory(std::string(columns, 'x') + " tail");

    const auto frame = scene.describe(kCanvas, {}, {}, console);

    EXPECT_TRUE(saysSomewhere(frame, "tail"));
}

TEST(ConsoleSceneTest, Describe_ListsTheNewestHistoryAboveTheField)
{
    constexpr std::size_t kShown = 8;

    const ConsoleScene scene;
    ConsoleState console;
    slideTo(console, kConsoleAnimTicks);

    ASSERT_EQ(kConsoleHistoryShown, kShown);

    for (std::size_t line = 0; line < kShown + 1; ++line)
    {
        console.pushHistory("line " + std::to_string(line));
    }

    const auto frame = scene.describe(kCanvas, {}, {}, console);

    EXPECT_FALSE(saysSomewhere(frame, "line 0"));

    for (std::size_t line = 1; line < kShown + 1; ++line)
    {
        EXPECT_TRUE(
            saysSomewhere(frame, "line " + std::to_string(line)));
    }
}
