#include "antwika/poker/TableScene.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Glyphs.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/holdem/CardText.hpp>
#include <antwika/holdem/Stage.hpp>
#include <antwika/ui/Alignment.hpp>
#include <antwika/ui/Painter.hpp>
#include <antwika/ui/Sizing.hpp>
#include <antwika/ui/Theme.hpp>

#include "antwika/poker/PokerAtlas.hpp"
#include "antwika/poker/TableWidgets.hpp"

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
        constexpr Color kRedSuit{.red = 176, .green = 32, .blue = 32};
        constexpr Color kBlackSuit{.red = 24, .green = 24, .blue = 28};
        constexpr Color kToAct{.red = 232, .green = 196, .blue = 72};
        constexpr Color kStackBar{.red = 72, .green = 160, .blue = 216};
        constexpr Color kInFront{.red = 224, .green = 176, .blue = 64};

        // An opaque white tint draws a slot of the atlas unchanged.
        constexpr Color kWhite{
            .red = 255, .green = 255, .blue = 255, .alpha = 255};

        // Every art rectangle is built here.
        // So no other line in this file casts a width into an origin.
        [[nodiscard]] Rect rectAt(
            std::uint32_t left,
            std::uint32_t top,
            std::uint32_t width,
            std::uint32_t height) noexcept
        {
            return Rect{
                .origin =
                    {.x = static_cast<std::int32_t>(left),
                     .y = static_cast<std::int32_t>(top)},
                .size = {.width = width, .height = height}};
        }

        [[nodiscard]] std::uint32_t without(
            std::uint32_t value, std::uint32_t amount) noexcept
        {
            return value > amount ? value - amount : 0;
        }

        // A rectangle pulled in on every side, never past nothing.
        [[nodiscard]] Rect deflated(
            Rect rect, std::uint32_t inset) noexcept
        {
            return Rect{
                .origin =
                    {.x = rect.origin.x
                          + static_cast<std::int32_t>(inset),
                     .y = rect.origin.y
                          + static_cast<std::int32_t>(inset)},
                .size = {
                    .width = without(rect.size.width, 2 * inset),
                    .height = without(rect.size.height, 2 * inset)}};
        }

        // A square hung on a rectangle's top-left corner.
        [[nodiscard]] Rect squareAt(
            Rect rect, std::uint32_t side) noexcept
        {
            return Rect{
                .origin = rect.origin,
                .size = {.width = side, .height = side}};
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
        [[nodiscard]] std::uint32_t minimumSeatRowHeight(
            std::uint32_t scale) noexcept
        {
            return 24 * scale;
        }

        // How tall one seat row is.
        // Half the canvas, shared out, unless that is unreadably thin.
        // The layout shrinks a table too tall for its window.
        // The art follows it there rather than repeating this sum.
        [[nodiscard]] std::uint32_t seatRowHeight(
            Size canvas, std::size_t seats) noexcept
        {
            const auto scale = scaleForCanvas(canvas);
            const auto rows = std::max<std::uint32_t>(
                1, static_cast<std::uint32_t>(seats));

            return std::max(
                minimumSeatRowHeight(scale), canvas.height / 2 / rows);
        }

        // A chip, a dealer button or a turn marker is one badge tall.
        // Two lines, so it reads as a token beside a line of text.
        [[nodiscard]] std::uint32_t badgeSide(
            std::uint32_t scale) noexcept
        {
            return 2 * kGlyphLineHeight * scale;
        }
    } // namespace

    void TableScene::draw(
        IRenderer &renderer,
        Size canvas,
        const TableSnapshot &snapshot,
        const ITexture *atlas) const
    {
        // ui::paint() deliberately neither clears nor presents.
        // So the felt behind the picture is still this scene's to lay.
        // It is still laid under the art, since a blit may be shaped.
        renderer.clear(kFelt);

        // The layout comes first because the art is placed from it.
        const auto frame = describe(canvas, snapshot);

        // No atlas is an ordinary state rather than a failure.
        // A test draws through a mock that was never handed one.
        if (atlas != nullptr)
        {
            for (const auto &blit :
                 describeArt(canvas, frame.rects, snapshot))
            {
                renderer.drawTexture(
                    *atlas, blit.source, blit.destination, blit.tint);
            }
        }

        ui::paint(renderer, frame.commands);
    }

    void TableScene::appendFelt(std::vector<ArtBlit> &art, Size canvas)
    {
        // Tiled rather than stretched.
        // The felt is textured, so blowing one up would show it.
        const auto tile = kAtlasSlotSize.width;
        const auto across = (canvas.width + tile - 1) / tile;
        const auto down = (canvas.height + tile - 1) / tile;

        for (std::uint32_t row = 0; row < down; ++row)
        {
            for (std::uint32_t column = 0; column < across; ++column)
            {
                art.push_back(ArtBlit{
                    .source = sourceOf(kFeltSlot),
                    .destination = rectAt(
                        column * tile, row * tile, tile, tile),
                    .tint = kWhite});
            }
        }
    }

    void TableScene::appendCard(
        std::vector<ArtBlit> &art,
        holdem::Card card,
        Rect destination,
        bool faceUp)
    {
        if (!faceUp)
        {
            art.push_back(ArtBlit{
                .source = sourceOf(kCardBackSlot),
                .destination = destination,
                .tint = kWhite});

            return;
        }

        art.push_back(ArtBlit{
            .source = sourceOf(kCardFaceSlot),
            .destination = destination,
            .tint = kWhite});

        // A rank glyph over a suit glyph, rather than one of 52 faces.
        // Both are drawn white, so the tint is what colours the suit.
        const auto ink = isRedSuit(card) ? kRedSuit : kBlackSuit;
        const auto width = destination.size.width;
        const auto height = destination.size.height;
        const auto glyph = width / 2;

        // gcov attributes one block to the line below.
        // It is the unwind cleanup for the temporary being pushed.
        // gcov marks it unreachable itself, as $$$$$ under `gcov -a`.
        // Every other line of this statement runs per face-up card.
        art.push_back(ArtBlit{ // GCOVR_EXCL_LINE
            .source = rankSourceOf(card),
            .destination = Rect{
                .origin = {
                    .x = destination.origin.x
                         + static_cast<std::int32_t>(width / 8),
                    .y = destination.origin.y
                         + static_cast<std::int32_t>(height / 10)},
                .size = {.width = glyph, .height = glyph}},
            .tint = ink});

        art.push_back(ArtBlit{
            .source = suitSourceOf(card),
            .destination = Rect{
                .origin = {
                    .x = destination.origin.x
                         + static_cast<std::int32_t>(width - glyph
                                                     - width / 8),
                    .y = destination.origin.y
                         + static_cast<std::int32_t>(height - glyph
                                                     - height / 10)},
                .size = {.width = glyph, .height = glyph}},
            .tint = ink});
    }

    void TableScene::appendBoard(
        std::vector<ArtBlit> &art,
        const WidgetRects &rects,
        const TableSnapshot &snapshot)
    {
        // The board is always shown face up: it is everybody's.
        for (std::size_t index = 0; index < snapshot.board.size();
             ++index)
        {
            const auto face = rects.find(widgets::boardCard(index));

            if (face.has_value())
            {
                appendCard(art, snapshot.board[index], *face, true);
            }
        }

        // A chip beside the pot, in the room the layout kept for it.
        const auto pot = rects.find(widgets::kPot);

        if (pot.has_value())
        {
            art.push_back(ArtBlit{
                .source = sourceOf(kChipSlot),
                .destination = *pot,
                .tint = kWhite});
        }
    }

    void TableScene::appendSeats(
        std::vector<ArtBlit> &art,
        const WidgetRects &rects,
        const TableSnapshot &snapshot)
    {
        for (std::size_t index = 0; index < snapshot.seats.size();
             ++index)
        {
            appendSeat(art, rects, snapshot, index);
        }
    }

    void TableScene::appendSeat(
        std::vector<ArtBlit> &art,
        const WidgetRects &rects,
        const TableSnapshot &snapshot,
        std::size_t index)
    {
        const auto row = rects.find(widgets::seat(index));

        // A seat the frame did not declare gets no furniture.
        if (!row.has_value())
        {
            return;
        }

        // The plate is the row itself, pulled in so its edge reads.
        const auto plate = deflated(*row, row->size.height / 8);

        // A plate for every seat, taken or not.
        // So the table shows how many places it has.
        art.push_back(ArtBlit{
            .source = sourceOf(kPlateSlot),
            .destination = plate,
            .tint = kWhite});

        const auto &seat = snapshot.seats[index];

        if (!seat.occupied)
        {
            return;
        }

        art.push_back(ArtBlit{
            .source = sourceOf(kChairSlot),
            .destination = squareAt(plate, plate.size.height),
            .tint = kWhite});

        if (seat.inHand)
        {
            // Face up only once every hand is over the same cards.
            const auto faceUp = snapshot.stage == Stage::Showdown;
            const auto first = widgets::firstHoleCard(index);

            for (std::size_t card = 0; card < seat.holeCards.size();
                 ++card)
            {
                const auto face =
                    rects.find(widgets::after(first, card));

                if (face.has_value())
                {
                    appendCard(
                        art, seat.holeCards[card], *face, faceUp);
                }
            }
        }

        const auto bet = rects.find(widgets::betBadge(index));

        if (bet.has_value())
        {
            art.push_back(ArtBlit{
                .source = sourceOf(kChipSlot),
                .destination = *bet,
                .tint = kWhite});
        }

        const auto button = rects.find(widgets::dealerBadge(index));

        if (button.has_value())
        {
            art.push_back(ArtBlit{
                .source = sourceOf(kDealerButtonSlot),
                .destination = *button,
                .tint = kWhite});
        }

        if (seat.isToAct)
        {
            // Over the chair.
            // Which is where an eye hunting for a turn already is.
            art.push_back(ArtBlit{
                .source = sourceOf(kToActSlot),
                .destination = squareAt(plate, plate.size.height / 2),
                .tint = kToAct});
        }
    }

    std::vector<ArtBlit> TableScene::describeArt(
        Size canvas,
        const WidgetRects &rects,
        const TableSnapshot &snapshot) const
    {
        std::vector<ArtBlit> art;

        // The felt is the one thing here the canvas still decides.
        // It covers the whole window and so belongs to no widget.
        appendFelt(art, canvas);
        appendBoard(art, rects, snapshot);
        appendSeats(art, rects, snapshot);

        return art;
        // The closing brace is the local vector's landing pad.
        // Nothing between its construction and the return throws.
    } // GCOVR_EXCL_LINE

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

        {
            const auto pot = ui.row(
                {.width = kFit,
                 .height = kFit,
                 .cross = Alignment::Center,
                 .gap = scale});

            // The chip is art, so all the layout does is keep it room.
            {
                const auto chip = ui.column(
                    {.width = fixedSize(badgeSide(scale)),
                     .height = fixedSize(badgeSide(scale)),
                     .id = widgets::kPot});
            }

            ui.label("pot " + std::to_string(snapshot.pot), kInk);
        }

        if (!snapshot.board.empty())
        {
            describeCards(
                ui, snapshot.board, scale, widgets::boardCard(0));
        }

        ui.spacer(kGrow);
    }

    void TableScene::describeSeats(
        Context &ui,
        Size canvas,
        const TableSnapshot &snapshot,
        std::uint32_t scale) const
    {
        const auto rowHeight =
            seatRowHeight(canvas, snapshot.seats.size());

        // A bar is sized against the canvas, not the row it sits in.
        // A child cannot ask what its container was given.
        const auto barRoom = canvas.width / 3;

        const auto largest = largestStack(snapshot);
        const auto showButton = snapshot.handsPlayed > 0;

        for (std::size_t index = 0; index < snapshot.seats.size();
             ++index)
        {
            describeSeat(
                ui,
                snapshot.seats[index],
                {.index = index,
                 .rowHeight = rowHeight,
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
        //
        // Named, because this is the rectangle the art plates.
        // The plate and the box are then one row.
        // Rather than two rows that were handed the same pitch.
        const auto border = ui.column(
            {.width = kGrow,
             .height = fixedSize(metrics.rowHeight),
             .background = seat.isToAct ? kToAct : kRail,
             .padding = metrics.scale,
             .gap = 0,
             .id = widgets::seat(metrics.index)});

        // Hoisted so the multiply owns a statement gcov counts.
        // Inline, it took a line note that never reached 100%.
        // The computation folds into the block after it.
        // The sibling above has no arithmetic, and so no note at all.
        const auto boxPadding = 2 * metrics.scale;

        const auto box = ui.column(
            {.width = kGrow,
             .height = kGrow,
             .background = kSeatBox,
             .padding = boxPadding,
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

        describeBadges(ui, seat, metrics);

        if (seat.inHand)
        {
            describeCards(
                ui,
                seat.holeCards,
                metrics.scale,
                widgets::firstHoleCard(metrics.index));
        }
    }

    void TableScene::describeBadges(
        Context &ui, const SeatSnapshot &seat, SeatMetrics metrics) const
    {
        // Both are art, so all the layout does is keep them room.
        // Between the details and the cards.
        // Which is where the art used to walk leftwards to find them.
        const auto side = badgeSide(metrics.scale);

        if (seat.roundCommitted > 0)
        {
            const auto chip = ui.column(
                {.width = fixedSize(side),
                 .height = fixedSize(side),
                 .id = widgets::betBadge(metrics.index)});
        }

        if (seat.isButton && metrics.showButton)
        {
            const auto button = ui.column(
                {.width = fixedSize(side),
                 .height = fixedSize(side),
                 .id = widgets::dealerBadge(metrics.index)});
        }
    }

    void TableScene::describeCards(
        Context &ui,
        std::span<const holdem::Card> cards,
        std::uint32_t scale,
        WidgetId first) const
    {
        const auto row =
            ui.row({.width = kFit, .height = kFit, .gap = scale});

        for (std::size_t index = 0; index < cards.size(); ++index)
        {
            describeCard(
                ui, cards[index], scale, widgets::after(first, index));
        }
    }

    void TableScene::describeCard(
        Context &ui,
        holdem::Card card,
        std::uint32_t scale,
        WidgetId id) const
    {
        // How big a card is is stated here and nowhere else.
        // The art blits into the rectangle this lays out.
        // So no second size is left anywhere to disagree with it.
        //
        // Two glyph cells of text, a cell of room on each side.
        // A line of room above and below.
        // Twenty-four glyph pixels by thirty-six: a playing card.
        // Which is the shape of one in the only metrics here.
        //
        // It carries no fill of its own.
        // The face is a slot of the atlas.
        // A colour over it would be the picture painted twice.
        // Hoisted so each multiply owns a statement gcov counts.
        // Inline, they take a line note that never reaches 100%.
        const auto sides = kGlyphAdvance * scale;
        const auto shoulder = kGlyphLineHeight * scale;

        const auto face = ui.column(
            {.width = kFit,
             .height = kFit,
             .cross = Alignment::Center,
             .padding = sides,
             .gap = 0,
             .id = id});

        ui.spacer(fixedSize(shoulder));
        ui.label(holdem::toString(card), suitColor(card));
        ui.spacer(fixedSize(shoulder));
    }

} // namespace antwika::poker
