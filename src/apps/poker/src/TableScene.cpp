#include "antwika/poker/TableScene.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Glyphs.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/TextLayout.hpp>
#include <antwika/holdem/CardText.hpp>
#include <antwika/holdem/Stage.hpp>

namespace antwika::poker
{

    using antwika::gfx::Color;
    using antwika::gfx::kGlyphAdvance;
    using antwika::gfx::kGlyphLineHeight;
    using antwika::gfx::Point;
    using antwika::gfx::textSize;
    using antwika::holdem::Card;
    using antwika::holdem::Suit;
    using antwika::holdem::suitOf;

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

        // Text below this is unreadable, above it wastes the window.
        constexpr std::uint32_t kCanvasPerPixel = 240;

        [[nodiscard]] std::uint32_t scaleFor(Size canvas) noexcept
        {
            return std::max(1U, canvas.height / kCanvasPerPixel);
        }

        [[nodiscard]] std::uint32_t lineHeight(std::uint32_t scale) noexcept
        {
            return kGlyphLineHeight * scale;
        }

        [[nodiscard]] std::uint32_t cardWidth(std::uint32_t scale) noexcept
        {
            // Two glyph cells for the rank and suit, one for padding.
            return 3 * kGlyphAdvance * scale;
        }

        [[nodiscard]] std::uint32_t cardHeight(std::uint32_t scale) noexcept
        {
            return 2 * lineHeight(scale);
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
        // Any shorter and a row draws over the row beneath it.
        [[nodiscard]] std::uint32_t seatRowHeight(
            std::uint32_t scale) noexcept
        {
            return 24 * scale;
        }

        void fillBorder(
            IRenderer &renderer,
            Rect box,
            std::uint32_t thickness,
            Color color)
        {
            renderer.drawRect(box, color);
            renderer.drawRect(
                Rect{
                    .origin =
                        {.x = box.origin.x + static_cast<std::int32_t>(
                                                 thickness),
                         .y = box.origin.y + static_cast<std::int32_t>(
                                                 thickness)},
                    .size =
                        {.width = box.size.width - 2 * thickness,
                         .height = box.size.height - 2 * thickness}},
                kSeatBox);
        }
    } // namespace

    void TableScene::draw(
        IRenderer &renderer, Size canvas, const TableSnapshot &snapshot) const
    {
        const auto scale = scaleFor(canvas);
        const auto margin = lineHeight(scale);

        renderer.clear(kFelt);

        // A rail around the felt, so the table reads as a table.
        renderer.drawRect(
            Rect{
                .origin = {.x = 0, .y = 0},
                .size = {.width = canvas.width, .height = margin / 2}},
            kRail);

        drawHeader(renderer, canvas, snapshot, scale);

        // The rows sit at the bottom and grow upward.
        // So a nine-seat table eats into the felt above it.
        // It does not run off the bottom of the window.
        const auto rows =
            std::max<std::uint32_t>(
                1, static_cast<std::uint32_t>(snapshot.seats.size()));
        const auto rowHeight =
            std::max(seatRowHeight(scale), canvas.height / 2 / rows);
        const auto used = rowHeight * rows;
        const auto top = canvas.height > used ? canvas.height - used : 0;

        // The board fills what is left between header and seats.
        // A fixed fraction is what a nine-seat table would draw over.
        const auto headerBottom = 2 * lineHeight(scale) + margin;
        drawBoard(
            renderer,
            Rect{
                .origin =
                    {.x = 0,
                     .y = static_cast<std::int32_t>(headerBottom)},
                .size =
                    {.width = canvas.width,
                     .height = top > headerBottom ? top - headerBottom : 0}},
            snapshot,
            scale);

        const auto largest = largestStack(snapshot);
        const auto showButton = snapshot.handsPlayed > 0;

        for (std::size_t index = 0; index < snapshot.seats.size(); ++index)
        {
            const auto offset = static_cast<std::uint32_t>(index);
            drawSeat(
                renderer,
                Rect{
                    .origin =
                        {.x = static_cast<std::int32_t>(margin),
                         .y = static_cast<std::int32_t>(
                             top + offset * rowHeight)},
                    .size =
                        {.width = canvas.width - 2 * margin,
                         .height = rowHeight - scale}},
                snapshot.seats[index],
                largest,
                showButton,
                scale);
        }
    }

    void TableScene::drawHeader(
        IRenderer &renderer,
        Size canvas,
        const TableSnapshot &snapshot,
        std::uint32_t scale) const
    {
        const auto margin = lineHeight(scale);
        const auto left = static_cast<std::int32_t>(margin);

        renderer.drawText(
            Point{.x = left, .y = static_cast<std::int32_t>(margin)},
            headerOf(snapshot),
            scale,
            kInk);

        const auto blinds = blindsOf(snapshot);
        const auto width = textSize(blinds, scale).width;

        renderer.drawText(
            Point{
                .x = static_cast<std::int32_t>(
                    canvas.width - margin - width),
                .y = static_cast<std::int32_t>(margin)},
            blinds,
            scale,
            kDim);
    }

    void TableScene::drawBoard(
        IRenderer &renderer,
        Rect area,
        const TableSnapshot &snapshot,
        std::uint32_t scale) const
    {
        // The pot line and the card row, centred together.
        const auto block = lineHeight(scale) + scale + cardHeight(scale);
        const auto slack =
            area.size.height > block ? (area.size.height - block) / 2 : 0;
        const auto potTop = static_cast<std::uint32_t>(area.origin.y) + slack;

        const auto pot = "pot " + std::to_string(snapshot.pot);
        const auto potWidth = textSize(pot, scale).width;

        renderer.drawText(
            Point{
                .x = static_cast<std::int32_t>(
                    (area.size.width - potWidth) / 2),
                .y = static_cast<std::int32_t>(potTop)},
            pot,
            scale,
            kInk);

        if (snapshot.board.empty())
        {
            return;
        }

        const auto step = cardWidth(scale) + scale;
        const auto cards = static_cast<std::uint32_t>(snapshot.board.size());
        const auto left = (area.size.width - (cards * step - scale)) / 2;
        const auto top = potTop + lineHeight(scale) + scale;

        for (std::size_t index = 0; index < snapshot.board.size(); ++index)
        {
            const auto offset = static_cast<std::uint32_t>(index);
            drawCard(
                renderer,
                Rect{
                    .origin =
                        {.x = static_cast<std::int32_t>(
                             left + offset * step),
                         .y = static_cast<std::int32_t>(top)},
                    .size =
                        {.width = cardWidth(scale),
                         .height = cardHeight(scale)}},
                snapshot.board[index],
                scale);
        }
    }

    void TableScene::drawSeat(
        IRenderer &renderer,
        Rect box,
        const SeatSnapshot &seat,
        Chips largestStack,
        bool showButton,
        std::uint32_t scale) const
    {
        // The border is all that marks whose turn it is.
        // So it is drawn even for a seat with nothing else to show.
        fillBorder(renderer, box, scale, seat.isToAct ? kToAct : kRail);

        const auto pad = 2 * scale;
        const auto left = box.origin.x + static_cast<std::int32_t>(pad);
        const auto textTop = box.origin.y + static_cast<std::int32_t>(pad);

        if (!seat.occupied)
        {
            renderer.drawText(
                Point{.x = left, .y = textTop}, "-- empty --", scale, kDim);
            return;
        }

        // Before the first deal the button is wherever Table put it.
        auto label = seat.name;
        if (seat.isButton && showButton)
        {
            label += " (D)";
        }

        renderer.drawText(
            Point{.x = left, .y = textTop},
            label,
            scale,
            seat.inHand ? kInk : kDim);

        renderer.drawText(
            Point{
                .x = left,
                .y = textTop + static_cast<std::int32_t>(lineHeight(scale))},
            std::to_string(seat.stack),
            scale,
            kInk);

        // The bar makes stacks comparable at a glance.
        const auto barTop = textTop
                            + static_cast<std::int32_t>(
                                2 * lineHeight(scale));
        const auto barRoom = box.size.width / 3;
        const auto barWidth =
            static_cast<std::uint32_t>(seat.stack * barRoom / largestStack);

        renderer.drawRect(
            Rect{
                .origin = {.x = left, .y = barTop},
                .size = {.width = barWidth, .height = scale * 2}},
            kStackBar);

        if (seat.roundCommitted > 0)
        {
            renderer.drawText(
                Point{
                    .x = left + static_cast<std::int32_t>(barRoom)
                         + static_cast<std::int32_t>(pad),
                    .y = textTop
                         + static_cast<std::int32_t>(lineHeight(scale))},
                "bet " + std::to_string(seat.roundCommitted),
                scale,
                kInFront);
        }

        if (!seat.inHand)
        {
            return;
        }

        const auto step = cardWidth(scale) + scale;
        const auto cards =
            static_cast<std::uint32_t>(seat.holeCards.size());
        const auto cardsLeft = static_cast<std::uint32_t>(box.origin.x)
                               + box.size.width - pad
                               - (cards * step - scale);

        for (std::size_t index = 0; index < seat.holeCards.size(); ++index)
        {
            const auto offset = static_cast<std::uint32_t>(index);
            drawCard(
                renderer,
                Rect{
                    .origin =
                        {.x = static_cast<std::int32_t>(
                             cardsLeft + offset * step),
                         .y = textTop},
                    .size =
                        {.width = cardWidth(scale),
                         .height = cardHeight(scale)}},
                seat.holeCards[index],
                scale);
        }
    }

    void TableScene::drawCard(
        IRenderer &renderer,
        Rect box,
        Card card,
        std::uint32_t scale) const
    {
        renderer.drawRect(box, kCardFace);

        const auto label = holdem::toString(card);
        const auto size = textSize(label, scale);

        renderer.drawText(
            Point{
                .x = box.origin.x
                     + static_cast<std::int32_t>(
                         (box.size.width - size.width) / 2),
                .y = box.origin.y
                     + static_cast<std::int32_t>(
                         (box.size.height - size.height) / 2)},
            label,
            scale,
            suitColor(card));
    }

} // namespace antwika::poker
