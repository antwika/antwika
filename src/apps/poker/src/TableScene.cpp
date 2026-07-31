#include "antwika/poker/TableScene.hpp"

#include <algorithm>
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

        // An opaque white tint draws a slot of the atlas unchanged.
        constexpr Color kWhite{
            .red = 255, .green = 255, .blue = 255, .alpha = 255};

        // Below this a card is fewer pixels than it has glyphs.
        constexpr std::uint32_t kMinimumCardWidth = 8;

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
        IRenderer &renderer,
        Size canvas,
        const TableSnapshot &snapshot,
        const ITexture *atlas) const
    {
        // ui::paint() deliberately neither clears nor presents.
        // So the felt behind the picture is still this scene's to lay.
        // It is still laid under the art, since a blit may be shaped.
        renderer.clear(kFelt);

        // No atlas is an ordinary state rather than a failure.
        // A test draws through a mock that was never handed one.
        if (atlas != nullptr)
        {
            for (const auto &blit : describeArt(canvas, snapshot))
            {
                renderer.drawTexture(
                    *atlas, blit.source, blit.destination, blit.tint);
            }
        }

        ui::paint(renderer, describe(canvas, snapshot).commands);
    }

    TableScene::ArtMetrics TableScene::artMetricsFor(
        Size canvas, const TableSnapshot &snapshot)
    {
        const auto scale = scaleForCanvas(canvas);
        const auto rows = std::max<std::uint32_t>(
            1, static_cast<std::uint32_t>(snapshot.seats.size()));

        // The same share of the canvas the ui rows take.
        // So the art lands under the text rather than beside it.
        const auto rowHeight =
            std::max(seatRowHeight(scale), canvas.height / 2 / rows);
        const auto seatRoom = rowHeight * rows;

        // A table with more seats than canvas has no room above them.
        const auto seatTop =
            seatRoom < canvas.height ? canvas.height - seatRoom : 0;

        const auto cardWidth =
            std::max<std::uint32_t>(kMinimumCardWidth, canvas.width / 18);
        const auto cardHeight = cardWidth * 3 / 2;

        // Centred in whatever the seats left, or hard against the top.
        const auto boardTop =
            seatTop > cardHeight ? (seatTop - cardHeight) / 2 : 0;

        return ArtMetrics{
            .cardWidth = cardWidth,
            .cardHeight = cardHeight,
            .rowHeight = rowHeight,
            .seatTop = seatTop,
            .boardTop = boardTop,
        };
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

        // The only block gcov attributes to the line below is the
        // unwind cleanup for the temporary being pushed, which it marks
        // unreachable itself ($$$$$ under `gcov -a`).
        // Every other line of the statement runs on every face-up card.
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
        Size canvas,
        const TableSnapshot &snapshot,
        ArtMetrics metrics)
    {
        const auto gap = std::max<std::uint32_t>(1, metrics.cardWidth / 8);
        const auto count =
            static_cast<std::uint32_t>(snapshot.board.size());
        const auto span = count * metrics.cardWidth
                          + (count == 0 ? 0 : (count - 1) * gap);

        // The board is always shown face up: it is everybody's.
        auto left = canvas.width > span ? (canvas.width - span) / 2 : 0;
        for (const auto card : snapshot.board)
        {
            appendCard(
                art,
                card,
                rectAt(
                    left,
                    metrics.boardTop,
                    metrics.cardWidth,
                    metrics.cardHeight),
                true);
            left += metrics.cardWidth + gap;
        }

        // A chip under the middle, which is where the pot is written.
        const auto chip = metrics.cardHeight / 2;
        art.push_back(ArtBlit{
            .source = sourceOf(kChipSlot),
            .destination = rectAt(
                canvas.width / 2 - chip / 2,
                metrics.boardTop + metrics.cardHeight + gap,
                chip,
                chip),
            .tint = kWhite});
    }

    void TableScene::appendSeats(
        std::vector<ArtBlit> &art,
        Size canvas,
        const TableSnapshot &snapshot,
        ArtMetrics metrics)
    {
        const auto inset = metrics.rowHeight / 8;
        const auto plateHeight = metrics.rowHeight - 2 * inset;
        const auto seatCardHeight =
            std::min(metrics.cardHeight, plateHeight);
        const auto seatCardWidth = seatCardHeight * 2 / 3;
        const auto marker =
            std::max<std::uint32_t>(1, seatCardHeight / 2);

        std::uint32_t index = 0;
        for (const auto &seat : snapshot.seats)
        {
            const auto top = metrics.seatTop + index * metrics.rowHeight;
            ++index;

            // A plate for every seat, taken or not.
            // So the table shows how many places it has.
            art.push_back(ArtBlit{
                .source = sourceOf(kPlateSlot),
                .destination = rectAt(
                    inset,
                    top + inset,
                    canvas.width - 2 * inset,
                    plateHeight),
                .tint = kWhite});

            if (!seat.occupied)
            {
                continue;
            }

            art.push_back(ArtBlit{
                .source = sourceOf(kChairSlot),
                .destination =
                    rectAt(inset, top + inset, plateHeight, plateHeight),
                .tint = kWhite});

            // Two cards at the right edge, then the furniture leftwards.
            auto right = canvas.width - inset;

            if (seat.inHand)
            {
                // Face up only once every hand is over the same cards.
                const auto faceUp = snapshot.stage == Stage::Showdown;
                for (const auto card : seat.holeCards)
                {
                    right -= seatCardWidth + inset;
                    appendCard(
                        art,
                        card,
                        rectAt(
                            right,
                            top + inset,
                            seatCardWidth,
                            seatCardHeight),
                        faceUp);
                }
            }

            if (seat.roundCommitted > 0)
            {
                right -= marker + inset;
                art.push_back(ArtBlit{
                    .source = sourceOf(kChipSlot),
                    .destination =
                        rectAt(right, top + inset, marker, marker),
                    .tint = kWhite});
            }

            if (seat.isButton && snapshot.handsPlayed > 0)
            {
                right -= marker + inset;
                art.push_back(ArtBlit{
                    .source = sourceOf(kDealerButtonSlot),
                    .destination =
                        rectAt(right, top + inset, marker, marker),
                    .tint = kWhite});
            }

            if (seat.isToAct)
            {
                // Over the chair.
                // Which is where an eye hunting for a turn already is.
                art.push_back(ArtBlit{
                    .source = sourceOf(kToActSlot),
                    .destination =
                        rectAt(inset, top + inset, marker, marker),
                    .tint = kToAct});
            }
        }
    }

    std::vector<ArtBlit> TableScene::describeArt(
        Size canvas, const TableSnapshot &snapshot) const
    {
        const auto metrics = artMetricsFor(canvas, snapshot);

        std::vector<ArtBlit> art;
        appendFelt(art, canvas);
        appendBoard(art, canvas, snapshot, metrics);
        appendSeats(art, canvas, snapshot, metrics);

        return art;
        // The closing brace is the unwind landing pad destroying the
        // local vector; nothing between its construction and the return
        // throws.
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
