#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/app/preview/DrawnPreview.hpp>
#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/ui/Pointer.hpp>

#include "antwika/sudoku/Board.hpp"
#include "antwika/sudoku/Messages.hpp"
#include "antwika/sudoku/PuzzleFile.hpp"
#include "antwika/sudoku/PuzzleState.hpp"
#include "antwika/sudoku/SudokuScene.hpp"

namespace
{
    using antwika::app::preview::drawnPreview;
    using antwika::gfx::IRenderer;
    using antwika::gfx::Size;
    using antwika::sudoku::Board;
    using antwika::sudoku::kDemoPuzzle;
    using antwika::sudoku::PuzzleState;
    using antwika::sudoku::SudokuScene;
    using antwika::ui::Pointer;

    constexpr antwika::sudoku::Translator kTranslator{
        antwika::i18n::kDefaultLocale};

    constexpr Size kCanvas{.width = 720, .height = 800};
}

TEST(BoardPreviewTest, Draw_WritesAPuzzleMidSolve)
{
    EXPECT_FALSE(
        drawnPreview(
            {.name = "sudoku",
             .title = "Antwika Sudoku",
             .canvas = kCanvas},
            [](IRenderer &renderer)
            {
                PuzzleState state;
                state.start(Board::parse(kDemoPuzzle));
                state.select(antwika::sudoku::Square{.row = 0, .col = 2});
                state.enter(4);
                state.select(antwika::sudoku::Square{.row = 4, .col = 4});

                const SudokuScene scene(kTranslator);
                const auto frame = scene.describe(kCanvas, Pointer{}, state);
                scene.draw(renderer, frame.commands);
            })
            .empty());
}
