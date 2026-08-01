#include <cstddef>
#include <variant>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/i18n/Translator.hpp>
#include <antwika/ui/DrawCommand.hpp>
#include <antwika/ui/Pointer.hpp>

#include <antwika/sudoku/Board.hpp>
#include <antwika/sudoku/PuzzleFile.hpp>
#include <antwika/sudoku/PuzzleState.hpp>
#include <antwika/sudoku/SudokuScene.hpp>
#include <antwika/sudoku/Widgets.hpp>

#include "WidgetCentre.hpp"

using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::Size;
using antwika::sudoku::Board;
using antwika::sudoku::kDemoPuzzle;
using antwika::sudoku::PuzzleState;
using antwika::sudoku::Square;
using antwika::sudoku::SudokuScene;
using antwika::sudoku::tests::squareCentre;
using antwika::sudoku::tests::widgetCentre;
using antwika::ui::DrawText;
using antwika::ui::FillRect;
using antwika::ui::Frame;
using antwika::ui::Pointer;
using ::testing::_;
using ::testing::AtLeast;
using ::testing::NiceMock;
namespace widgets = antwika::sudoku::widgets;

namespace
{
    // The locale is a constant of the build, so a test may name one.
    constexpr antwika::i18n::Translator kTranslator{
        antwika::i18n::kDefaultLocale};

    constexpr Size kCanvas{.width = 720, .height = 800};

    [[nodiscard]] PuzzleState started()
    {
        PuzzleState state;
        state.start(Board::parse(kDemoPuzzle));
        return state;
    }

    [[nodiscard]] std::size_t countOf(
        const Frame &frame, const bool text)
    {
        std::size_t found = 0;

        for (const auto &command : frame.commands)
        {
            if (std::holds_alternative<DrawText>(command) == text)
            {
                ++found;
            }
        }

        return found;
    }

    [[nodiscard]] Pointer pressAt(const antwika::gfx::Point at)
    {
        return Pointer{
            .position = at, .down = true, .pressed = true};
    }

    TEST(SudokuSceneTest, Describe_AnswersTheSameThingEveryTime)
    {
        const SudokuScene scene{kTranslator};
        const auto state = started();

        const auto first = scene.describe(kCanvas, {}, state);
        const auto second = scene.describe(kCanvas, {}, state);

        EXPECT_EQ(first.commands, second.commands);
        EXPECT_EQ(first.rects, second.rects);
    }

    TEST(SudokuSceneTest, Describe_ReportsWhereTheBoardAndButtonWent)
    {
        const SudokuScene scene{kTranslator};
        const auto frame = scene.describe(kCanvas, {}, started());

        ASSERT_TRUE(frame.rects.find(widgets::kBoard).has_value());
        ASSERT_TRUE(frame.rects.find(widgets::kSolve).has_value());

        // The bar is above the grid.
        // Which is the one thing the two rectangles must agree on.
        EXPECT_LT(
            frame.rects.find(widgets::kSolve)->origin.y,
            frame.rects.find(widgets::kBoard)->origin.y);
    }

    TEST(SudokuSceneTest, Describe_DrawsOneDigitPerFilledSquare)
    {
        const SudokuScene scene{kTranslator};

        PuzzleState empty;
        empty.start(Board{});

        const auto bare = countOf(scene.describe(kCanvas, {}, empty), true);
        const auto demo =
            countOf(scene.describe(kCanvas, {}, started()), true);

        // The demo puzzle has thirty clues.
        // And the bar's own three lines are in both counts.
        EXPECT_EQ(demo - bare, 30U);
    }

    TEST(SudokuSceneTest, Describe_DrawsEverySquareAndTheHeavierRules)
    {
        const SudokuScene scene{kTranslator};
        const auto frame = scene.describe(kCanvas, {}, started());

        // Eighty-one squares, the sheet behind them, and four rules.
        // Plus whatever the bar itself filled in.
        EXPECT_GE(countOf(frame, false), 86U);
    }

    TEST(SudokuSceneTest, Describe_DrawsNoGridWhereNoneFits)
    {
        const SudokuScene scene{kTranslator};
        const auto frame = scene.describe(
            Size{.width = 40, .height = 40}, {}, started());

        EXPECT_LT(countOf(frame, false), 81U);
    }

    TEST(SudokuSceneTest, Describe_KeepsADigitInsideAVerySmallSquare)
    {
        // A canvas so narrow that a square is smaller than its digit.
        // Which is where an unsigned centring offset would wrap round.
        // And post the digit an enormous distance off the canvas.
        const SudokuScene scene{kTranslator};
        const auto narrow = Size{.width = 30, .height = 400};
        const auto frame = scene.describe(narrow, {}, started());

        ASSERT_GT(countOf(frame, true), 3U);

        for (const auto &command : frame.commands)
        {
            const auto *text = std::get_if<DrawText>(&command);

            if (text == nullptr)
            {
                continue;
            }

            EXPECT_LE(
                text->origin.x,
                static_cast<std::int32_t>(narrow.width));
            EXPECT_LE(
                text->origin.y,
                static_cast<std::int32_t>(narrow.height));
        }
    }

    TEST(SudokuSceneTest, Describe_ShowsWhichSquareIsPicked)
    {
        const SudokuScene scene{kTranslator};
        auto state = started();

        const auto before = scene.describe(kCanvas, {}, state);
        state.select(Square{.row = 4, .col = 4});
        const auto after = scene.describe(kCanvas, {}, state);

        EXPECT_NE(before.commands, after.commands);
    }

    TEST(SudokuSceneTest, Describe_TellsAClueFromWhatSomebodyTyped)
    {
        const SudokuScene scene{kTranslator};

        PuzzleState asClue;
        Board one;
        one.set(0, 0, 7);
        asClue.start(one);

        PuzzleState asTyped;
        asTyped.start(Board{});
        asTyped.write(Square{.row = 0, .col = 0}, 7);

        EXPECT_NE(
            scene.describe(kCanvas, {}, asClue).commands,
            scene.describe(kCanvas, {}, asTyped).commands);
    }

    TEST(SudokuSceneTest, Describe_ReportsAPressOnTheSolveButton)
    {
        const SudokuScene scene{kTranslator};
        const auto state = started();
        const auto where = widgetCentre(
            scene.describe(kCanvas, {}, state), widgets::kSolve);

        ASSERT_TRUE(where.has_value());

        const auto frame =
            scene.describe(kCanvas, pressAt(*where), state);

        EXPECT_EQ(frame.interactions.activated, widgets::kSolve);
    }

    TEST(SudokuSceneTest, Describe_ReportsAPressOnTheGrid)
    {
        const SudokuScene scene{kTranslator};
        const auto state = started();
        const auto where = squareCentre(
            scene.describe(kCanvas, {}, state),
            Square{.row = 0, .col = 2});

        ASSERT_TRUE(where.has_value());

        const auto frame =
            scene.describe(kCanvas, pressAt(*where), state);

        EXPECT_EQ(frame.interactions.activated, widgets::kBoard);
    }

    TEST(SudokuSceneTest, Draw_ClearsAndPaintsWhatItIsHanded)
    {
        const SudokuScene scene{kTranslator};
        NiceMock<MockRenderer> renderer;

        EXPECT_CALL(renderer, clear(_)).Times(1);
        EXPECT_CALL(renderer, drawRect(_, _)).Times(AtLeast(81));
        EXPECT_CALL(renderer, drawText(_, _, _, _)).Times(AtLeast(30));

        scene.draw(
            renderer, scene.describe(kCanvas, {}, started()).commands);
    }
} // namespace
