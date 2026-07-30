#include "antwika/poker/TableScene.hpp"

#include <algorithm>
#include <cstdint>
#include <span>
#include <string>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Glyphs.hpp>
#include <antwika/holdem/CardText.hpp>
#include <antwika/holdem/Stage.hpp>
#include <antwika/ui/Alignment.hpp>
#include <antwika/ui/Painter.hpp>
#include <antwika/ui/Sizing.hpp>
#include <antwika/ui/Theme.hpp>

namespace antwika::poker
{

    using antwika::gfx::Color;
    using antwika::gfx::kGlyphAdvance;
    using antwika::gfx::kGlyphLineHeight;
    using antwika::holdem::Card;
    using antwika::holdem::Suit;
    using antwika::holdem::suitOf;
    using antwika::ui::Alignment;
    using antwika::ui::fixedSize;
    using antwika::ui::kFit;
    using antwika::ui::kGrow;
    using antwika::ui::scaledTheme;
    using antwika::ui::scaleForCanvas;
    using antwika::ui::Theme;

    namespace
    {
        constexpr Color kFelt{.red = 12, .green = 68, .blue = 44};
        constexpr Color kRail{.red = 40, .green = 26, .blue = 18};
        constexpr Color kSeatBox{.red = 16, .green = 50, .blue = 36};
        constexpr Color kInk{.red = 232, .green = 236, .blue = 232};
        constexpr Color kDim{.red = 120, .green = 140, .blue = 128};
        constexpr Color kCardFace{.red = 240, .green = 240, .blue = 232};
        constexpr Color kRedSuit{.red = 176, .green = 32, .blue = 32};
        constexpr Color kBlackSuit{.red = 24, .green = 24, .blue = 28};
        constexpr Color kToAct{.red = 232, .green = 196, .blue = 72};
        constexpr Color kStackBar{.red = 72, .green = 160, .blue = 216};
        constexpr Color kInFront{.red = 224, .green = 176, .blue = 64};

        // Two glyph cells for the rank and the suit, one for padding.
        [[nodiscard]] std::uint32_t cardWidth(std::uint32_t scale) noexcept
        {
            return 3 * kGlyphAdvance * scale;
        }

        [[nodiscard]] std::uint32_t cardHeight(std::uint32_t scale) noexcept
        {
            return 2 * kGlyphLineHeight * scale;
        }

        [[nodiscard]] Color suitColor(Card card) noexcept
        {
            const auto suit = suitOf(card);
            const auto red =
                suit == Suit::Hearts || suit == Suit::Diamonds;
            return red ? kRedSuit : kBlackSuit;
        }

        [[nodiscard]] std::string headerOf(const TableSnapshot &snapshot)
        {
            if (snapshot.handsPlayed == 0)
            {
                return snapshot.tableName + " -- waiting for players";
            }

            return snapshot.tableName + " -- hand "
                   + std::to_string(snapshot.handsPlayed) + " -- "
                   + std::string(holdem::toString(snapshot.stage));
        }

        [[nodiscard]] std::string blindsOf(const TableSnapshot &snapshot)
        {
            return "blinds " + std::to_string(snapshot.blinds.small) + "/"
                   + std::to_string(snapshot.blinds.big);
        }

        [[nodiscard]] Chips largestStack(const TableSnapshot &snapshot)
        {
            Chips largest = 0;
            for (const auto &seat : snapshot.seats)
            {
                largest = std::max(largest, seat.stack);
            }
            // Never zero, so a bar width is always divisible.
            return std::max<Chips>(largest, 1);
        }

        // Top padding, the name, the stack, the bar, bottom padding.
        // Any shorter and a row has nothing left to show them in.
        [[nodiscard]] std::uint32_t seatRowHeight(
            std::uint32_t scale) noexcept
        {
            return 24 * scale;
        }
    } // namespace

    void TableScene::draw(
        IRenderer &renderer, Size canvas, const TableSnapshot &snapshot) const
    {
        // ui::paint() deliberately neither clears nor presents.
        // So the felt behind the picture is still this scene's to lay.
        renderer.clear(kFelt);

        ui::paint(renderer, describe(canvas, snapshot).commands);
    }

    Frame TableScene::describe(
        Size canvas, const TableSnapshot &snapshot) const
    {
        const auto scale = scaleForCanvas(canvas);

        Context ui{canvas, scaledTheme(Theme{}, scale)};

        const auto margin = kGlyphLineHeight * scale;

        {
            // Nothing here is spaced by the theme's gap.
            // The rail sits hard against the top of the canvas.
            // The body fills whatever it leaves.
            const auto table = ui.column(
                {.width = kGrow, .height = kGrow, .gap = 0});

            // A rail across the top, so the table reads as a table.
            {
                const auto rail = ui.row(
                    {.width = kGrow,
                     .height = fixedSize(margin / 2),
                     .background = kRail});
            }

            {
                const auto body = ui.column(
                    {.width = kGrow,
                     .height = kGrow,
                     .padding = margin,
                     .gap = 0});

                describeHeader(ui, snapshot);
                describeBoard(ui, snapshot, scale);
                describeSeats(ui, canvas, snapshot, scale);
            }
        }

        return ui.finish();
    }

    void TableScene::describeHeader(
        Context &ui, const TableSnapshot &snapshot) const
    {
        const auto header = ui.row({.width = kGrow});

        ui.label(headerOf(snapshot), kInk);
        // The growing gap between them is what pins the blinds right.
        ui.spacer(kGrow);
        ui.label(blindsOf(snapshot), kDim);
    }

    void TableScene::describeBoard(
        Context &ui,
        const TableSnapshot &snapshot,
        std::uint32_t scale) const
    {
        // The board takes whatever the header and the seats leave.
        // The two growing spacers hold its content in the middle.
        const auto board = ui.column(
            {.width = kGrow,
             .height = kGrow,
             .cross = Alignment::Center,
             .gap = scale});

        ui.spacer(kGrow);
        ui.label("pot " + std::to_string(snapshot.pot), kInk);

        if (!snapshot.board.empty())
        {
            describeCards(ui, snapshot.board, scale);
        }

        ui.spacer(kGrow);
    }

    void TableScene::describeSeats(
        Context &ui,
        Size canvas,
        const TableSnapshot &snapshot,
        std::uint32_t scale) const
    {
        const auto rows =
            std::max<std::uint32_t>(
                1, static_cast<std::uint32_t>(snapshot.seats.size()));

        // Half the canvas, shared out, unless that is unreadably thin.
        // A table too tall for the window is shrunk to fit by the layout.
        // Which is the guard this file used to write by hand.
        const auto rowHeight =
            std::max(seatRowHeight(scale), canvas.height / 2 / rows);

        // A bar is sized against the canvas, not the row it sits in.
        // A child cannot ask what its container was given.
        const auto barRoom = canvas.width / 3;

        const auto largest = largestStack(snapshot);
        const auto showButton = snapshot.handsPlayed > 0;

        for (const auto &seat : snapshot.seats)
        {
            describeSeat(
                ui,
                seat,
                {.rowHeight = rowHeight,
                 .barRoom = barRoom,
                 .largestStack = largest,
                 .scale = scale,
                 .showButton = showButton});
        }
    }

    void TableScene::describeSeat(
        Context &ui, const SeatSnapshot &seat, SeatMetrics metrics) const
    {
        // The border is all that marks whose turn it is.
        // So it is drawn even for a seat with nothing else to show.
        // A panel inset by one pixel inside a filled one is the border:
        // no arithmetic, and no way for the two to disagree.
        const auto border = ui.column(
            {.width = kGrow,
             .height = fixedSize(metrics.rowHeight),
             .background = seat.isToAct ? kToAct : kRail,
             .padding = metrics.scale,
             .gap = 0});

        const auto box = ui.column(
            {.width = kGrow,
             .height = kGrow,
             .background = kSeatBox,
             .padding = 2 * metrics.scale,
             .gap = 0});

        if (!seat.occupied)
        {
            ui.label("-- empty --", kDim);

            return;
        }

        const auto row = ui.row({.width = kGrow, .height = kGrow});

        {
            const auto details =
                ui.column({.width = kGrow, .height = kGrow, .gap = 0});

            // Before the first deal the button is wherever Table put it.
            auto label = seat.name;
            if (seat.isButton && metrics.showButton)
            {
                label += " (D)";
            }

            ui.label(label, seat.inHand ? kInk : kDim);

            {
                const auto stack =
                    ui.row({.width = kGrow, .gap = metrics.barRoom});

                ui.label(std::to_string(seat.stack), kInk);

                if (seat.roundCommitted > 0)
                {
                    ui.label(
                        "bet " + std::to_string(seat.roundCommitted),
                        kInFront);
                }
            }

            // The bar makes stacks comparable at a glance.
            // A filled container with no children is all it is.
            {
                const auto bar = ui.row(
                    {.width = fixedSize(static_cast<std::uint32_t>(
                         seat.stack * metrics.barRoom
                         / metrics.largestStack)),
                     .height = fixedSize(2 * metrics.scale),
                     .background = kStackBar});
            }

            ui.spacer(kGrow);
        }

        if (seat.inHand)
        {
            describeCards(ui, seat.holeCards, metrics.scale);
        }
    }

    void TableScene::describeCards(
        Context &ui,
        std::span<const holdem::Card> cards,
        std::uint32_t scale) const
    {
        const auto row =
            ui.row({.width = kFit, .height = kFit, .gap = scale});

        for (const auto card : cards)
        {
            describeCard(ui, card, scale);
        }
    }

    void TableScene::describeCard(
        Context &ui, holdem::Card card, std::uint32_t scale) const
    {
        // Centred across by the alignment and down by the two spacers.
        // The third of the three sites this file used to centre by hand.
        const auto face = ui.column(
            {.width = fixedSize(cardWidth(scale)),
             .height = fixedSize(cardHeight(scale)),
             .cross = Alignment::Center,
             .background = kCardFace,
             .gap = 0});

        ui.spacer(kGrow);
        ui.label(holdem::toString(card), suitColor(card));
        ui.spacer(kGrow);
    }

} // namespace antwika::poker
