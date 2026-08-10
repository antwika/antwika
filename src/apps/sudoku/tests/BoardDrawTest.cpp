#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/ui/Pointer.hpp>

#include "antwika/sudoku/Board.hpp"
#include "antwika/sudoku/Messages.hpp"
#include "antwika/sudoku/PuzzleFile.hpp"
#include "antwika/sudoku/PuzzleState.hpp"
#include "antwika/sudoku/SudokuScene.hpp"

namespace
{
    using antwika::gfx::Size;
    using antwika::gfx::mocks::MockRenderer;
    using antwika::sudoku::Board;
    using antwika::sudoku::kDemoPuzzle;
    using antwika::sudoku::PuzzleState;
    using antwika::sudoku::SudokuScene;
    using antwika::ui::Pointer;
    using ::testing::_;
    using ::testing::AtLeast;
    using ::testing::NiceMock;

    constexpr antwika::sudoku::Translator kTranslator{
        antwika::i18n::kDefaultLocale};

    constexpr Size kCanvas{.width = 720, .height = 800};
}

TEST(BoardDrawTest, Draw_DrawsAPuzzleMidSolve)
{
    NiceMock<MockRenderer> renderer;

    EXPECT_CALL(renderer, drawRect(_, _)).Times(AtLeast(1));

    PuzzleState state;
    state.start(Board::parse(kDemoPuzzle));
    state.select(antwika::sudoku::Square{.row = 0, .col = 2});
    state.enter(4);
    state.select(antwika::sudoku::Square{.row = 4, .col = 4});

    const SudokuScene scene(kTranslator);
    const auto frame = scene.describe(kCanvas, Pointer{}, state);

    scene.draw(renderer, frame.commands);
}
