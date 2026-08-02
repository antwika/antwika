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
        // The window behind the picture, not the theme's business.
        constexpr Color kBackdrop{.red = 12, .green = 14, .blue = 20};

        // Behind the whole grid.
        // So it shows through as every line between two squares.
        constexpr Color kRule{.red = 90, .green = 100, .blue = 120};

        constexpr Color kSquare{.red = 26, .green = 28, .blue = 36};
        constexpr Color kClueSquare{.red = 38, .green = 40, .blue = 50};
        constexpr Color kPicked{.red = 62, .green = 78, .blue = 60};

        constexpr Color kClueInk{.red = 190, .green = 198, .blue = 210};
        constexpr Color kOwnInk{.red = 120, .green = 220, .blue = 150};

        // How much of a square's edge each of the two rules takes.
        // In pixels of a square rather than of the window.
        // So a grid drawn into a small area keeps its proportions.
        constexpr std::uint32_t kHairlineIn = 20;
        constexpr std::uint32_t kBoxRuleIn = 8;

        // Where the heavier rules fall: every third column and row.
        constexpr std::size_t kBox = 3;

        [[nodiscard]] std::uint32_t atLeastOne(
            const std::uint32_t value) noexcept
        {
            return value == 0 ? 1 : value;
        }

        /**
         * @brief How much of a square's edge the thin rule takes.
         *
         * Capped so a square keeps at least one pixel of its own: the
         * inset is taken off both sides, and a grid squeezed into an
         * area a few pixels across would otherwise subtract more width
         * than a square has and wrap round to an enormous one.
         *
         * @param cell A square's edge, at least one pixel.
         * @return The inset, which may be none at all.
         */
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

        /**
         * @brief Offset that centres one extent inside another.
         *
         * Nothing is centred outside its square: a grid squeezed small
         * enough that a digit is wider than the square holding it would
         * otherwise wrap an unsigned subtraction round and post the
         * digit an enormous distance away.
         *
         * @param outer The room available.
         * @param inner What is being placed in it.
         * @return Where to put it, never before the start.
         */
        [[nodiscard]] std::int32_t centred(
            const std::uint32_t outer, const std::uint32_t inner) noexcept
        {
            if (inner >= outer)
            {
                return 0;
            }

            return static_cast<std::int32_t>((outer - inner) / 2);
        }

        /**
         * @brief Put one digit in the middle of its square.
         *
         * Measured with gfx::textSize(), which is the only thing that
         * knows how wide a string is drawn: a glyph's interior is not a
         * public concept, and a caller working one out from a font's
         * metrics would be a second measurement to drift from the one
         * the renderer uses.
         *
         * @param frame The frame to append to.
         * @param square The square's rectangle.
         * @param digit The digit to draw.
         * @param scale Pixels per glyph pixel.
         * @param ink The colour to draw it in.
         */
        void appendDigit(
            Frame &frame,
            const Rect square,
            const int digit,
            const std::uint32_t scale,
            const Color ink)
        {
            const std::string text = std::to_string(digit);
            const auto measured = antwika::gfx::textSize(text, scale);

            // The excluded lines below are the allocator's alone.
            // A DrawText owns a string, and push_back may grow.
            // One character never leaves the short-string buffer.
            // See docs/confirming-unreachable-branches.md.
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

        /**
         * @brief Draw the heavier rules between the nine boxes.
         * @param frame The frame to append to.
         * @param layout Where the grid lies.
         * @param thickness How wide a rule is.
         */
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

        /**
         * @brief Draw the grid into the area the bar left for it.
         *
         * Appended after ui::Context::finish() rather than declared
         * into the layout, following ui_demo::appendMarker(): what a
         * square looks like is this application's art, and the only
         * place its area is certain is the layout that has just been
         * arranged.
         *
         * @param frame The frame to append to, whose rects are read.
         * @param state The puzzle and the selection.
         */
        void appendBoard(Frame &frame, const PuzzleState &state)
        {
            // An id every frame declares, so the fallback is dead.
            // No grid fits a bare rectangle, so nothing is drawn.
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
    } // namespace

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

            // Named and unfilled, so it reports where it went.
            // A press inside it arrives as an activation.
            // Which this application then maps to a square.
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

} // namespace antwika::sudoku
