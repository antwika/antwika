#include "antwika/sudoku/SudokuScene.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Glyphs.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/TextLayout.hpp>
#include <antwika/ui/Alignment.hpp>
#include <antwika/ui/ButtonSpec.hpp>
#include <antwika/ui/ContainerSpec.hpp>
#include <antwika/ui/Context.hpp>
#include <antwika/ui/DrawCommand.hpp>
#include <antwika/ui/Painter.hpp>
#include <antwika/ui/Sizing.hpp>
#include <antwika/ui/Theme.hpp>

#include "antwika/sudoku/BoardLayout.hpp"
#include "antwika/sudoku/MessageId.hpp"
#include "antwika/sudoku/Messages.hpp"
#include "antwika/sudoku/Status.hpp"
#include "antwika/sudoku/Widgets.hpp"

namespace antwika::sudoku
{

    using antwika::gfx::Color;
    using antwika::gfx::Rect;
    using antwika::ui::Alignment;
    using antwika::ui::Context;
    using antwika::ui::DrawText;
    using antwika::ui::FillRect;
    using antwika::ui::kGrow;
    using antwika::ui::scaledTheme;
    using antwika::ui::scaleForCanvas;
    using antwika::ui::Theme;

    namespace
    {
        constexpr Color kBackdrop{.red = 12, .green = 14, .blue = 20};

        constexpr Color kRule{.red = 90, .green = 100, .blue = 120};

        constexpr Color kSquare{.red = 26, .green = 28, .blue = 36};
        constexpr Color kClueSquare{.red = 38, .green = 40, .blue = 50};
        constexpr Color kPicked{.red = 62, .green = 78, .blue = 60};

        constexpr Color kClueInk{.red = 190, .green = 198, .blue = 210};
        constexpr Color kOwnInk{.red = 120, .green = 220, .blue = 150};

        constexpr std::uint32_t kHairlineIn = 20;
        constexpr std::uint32_t kBoxRuleIn = 8;

        constexpr std::size_t kBox = 3;

        [[nodiscard]] std::uint32_t atLeastOne(
            const std::uint32_t value) noexcept
        {
            return value == 0 ? 1 : value;
        }

        [[nodiscard]] std::uint32_t hairlineFor(
            const std::uint32_t cell) noexcept
        {
            const auto wanted = atLeastOne(cell / kHairlineIn);
            const auto most = (cell - 1) / 2;

            return wanted < most ? wanted : most;
        }

        [[nodiscard]] Color fillFor(
            const PuzzleState &state, const Square square)
        {
            if (state.selected() == square)
            {
                return kPicked;
            }

            return state.isGiven(square) ? kClueSquare : kSquare;
        }

        [[nodiscard]] std::int32_t centred(
            const std::uint32_t outer, const std::uint32_t inner) noexcept
        {
            if (inner >= outer)
            {
                return 0;
            }

            return static_cast<std::int32_t>((outer - inner) / 2);
        }

        void appendDigit(
            Frame &frame,
            const Rect square,
            const int digit,
            const std::uint32_t scale,
            const Color ink)
        {
            const std::string text = std::to_string(digit);
            const auto measured = antwika::gfx::textSize(text, scale);

            // GCOVR_EXCL_START
            frame.commands.push_back(DrawText{
                .origin =
                    {.x = square.origin.x
                          + centred(square.size.width, measured.width),
                     .y = square.origin.y
                          + centred(
                              square.size.height, measured.height)},
                .text = text,
                .scale = scale,
                .color = ink});
            // GCOVR_EXCL_STOP
        }

        void appendBoxRules(
            Frame &frame,
            const BoardLayout &layout,
            const std::uint32_t thickness)
        {
            const auto span = layout.cell
                              * static_cast<std::uint32_t>(Board::kSize);
            const auto half = static_cast<std::int32_t>(thickness / 2);

            for (std::size_t at = kBox; at < Board::kSize; at += kBox)
            {
                const auto along = static_cast<std::int32_t>(
                    layout.cell * static_cast<std::uint32_t>(at));

                frame.commands.push_back(FillRect{
                    .rect =
                        {.origin =
                             {.x = layout.origin.x + along - half,
                              .y = layout.origin.y},
                         .size = {.width = thickness, .height = span}},
                    .color = kRule});

                frame.commands.push_back(FillRect{
                    .rect =
                        {.origin =
                             {.x = layout.origin.x,
                              .y = layout.origin.y + along - half},
                         .size = {.width = span, .height = thickness}},
                    .color = kRule});
            }
        }

        void appendBoard(Frame &frame, const PuzzleState &state)
        {
            const auto layout = layoutFor(
                frame.rects.find(widgets::kBoard).value_or(Rect{}));

            if (!layout.has_value())
            {
                return;
            }

            const auto span = layout->cell
                              * static_cast<std::uint32_t>(Board::kSize);
            const auto hairline = hairlineFor(layout->cell);
            const auto digitScale = atLeastOne(
                layout->cell / (2 * antwika::gfx::kGlyphLineHeight));

            frame.commands.push_back(FillRect{
                .rect = {.origin = layout->origin,
                         .size = {.width = span, .height = span}},
                .color = kRule});

            for (std::size_t row = 0; row < Board::kSize; ++row)
            {
                for (std::size_t col = 0; col < Board::kSize; ++col)
                {
                    const Square square{.row = row, .col = col};
                    const auto area = squareRect(*layout, square);

                    frame.commands.push_back(FillRect{
                        .rect =
                            {.origin =
                                 {.x = area.origin.x
                                       + static_cast<std::int32_t>(
                                           hairline),
                                  .y = area.origin.y
                                       + static_cast<std::int32_t>(
                                           hairline)},
                             .size =
                                 {.width = area.size.width
                                           - 2 * hairline,
                                  .height = area.size.height
                                            - 2 * hairline}},
                        .color = fillFor(state, square)});
                }
            }

            appendBoxRules(
                frame, *layout, atLeastOne(layout->cell / kBoxRuleIn));

            for (std::size_t row = 0; row < Board::kSize; ++row)
            {
                for (std::size_t col = 0; col < Board::kSize; ++col)
                {
                    const Square square{.row = row, .col = col};
                    const auto digit = state.board().at(row, col);

                    if (!digit.has_value())
                    {
                        continue;
                    }

                    appendDigit(
                        frame,
                        squareRect(*layout, square),
                        *digit,
                        digitScale,
                        state.isGiven(square) ? kClueInk : kOwnInk);
                }
            }
        }
    }

    SudokuScene::SudokuScene(const Translator &translator)
        : translator(translator)
    {
    }

    Frame SudokuScene::describe(
        const Size canvas,
        const Pointer pointer,
        const PuzzleState &state) const
    {
        Context ui{
            canvas,
            scaledTheme(Theme{}, scaleForCanvas(canvas)),
            pointer};

        {
            const auto screen =
                ui.column({.width = kGrow, .height = kGrow});

            {
                const auto header = ui.panel({.width = kGrow});

                {
                    const auto row = ui.row(
                        {.width = kGrow, .cross = Alignment::Center});

                    ui.label(translator.text(MessageId::Title));
                    ui.spacer(kGrow);
                    ui.button(
                        translator.text(MessageId::SolveButton),
                        {.id = widgets::kSolve});
                }

                ui.label(
                    translator.text(statusNameId(state.status())),
                    ui.theme().muted);
            }

            {
                const auto board = ui.column(
                    {.width = kGrow,
                     .height = kGrow,
                     .id = widgets::kBoard});
            }
        }

        auto frame = ui.finish();
        appendBoard(frame, state);

        return frame;
    }

    void SudokuScene::draw(
        IRenderer &renderer, const DrawList &picture) const
    {
        renderer.clear(kBackdrop);
        antwika::ui::paint(renderer, picture);
    }

}
