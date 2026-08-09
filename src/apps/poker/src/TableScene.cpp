#include "antwika/poker/TableScene.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
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

        constexpr Color kWhite{
            .red = 255, .green = 255, .blue = 255, .alpha = 255};

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
            return std::max<Chips>(largest, 1);
        }

        [[nodiscard]] std::uint32_t badgeSide(
            std::uint32_t scale) noexcept
        {
            return 2 * kGlyphLineHeight * scale;
        }

        [[nodiscard]] std::uint32_t cardHeight(
            std::uint32_t scale) noexcept
        {
            return (3 * kGlyphLineHeight * scale)
                   + (2 * kGlyphAdvance * scale);
        }

        [[nodiscard]] std::uint32_t seatChrome(
            std::uint32_t scale) noexcept
        {
            return 6 * scale;
        }

        constexpr std::uint32_t kSeatNameCells = 11;

        [[nodiscard]] std::uint32_t seatWidth(
            std::uint32_t scale) noexcept
        {
            return (kSeatNameCells * kGlyphAdvance * scale)
                   + (2 * badgeSide(scale)) + seatChrome(scale);
        }

        [[nodiscard]] std::uint32_t seatHeight(
            std::uint32_t scale) noexcept
        {
            return (3 * kGlyphLineHeight * scale) + (2 * scale)
                   + cardHeight(scale) + seatChrome(scale);
        }

        struct RingPlan final
        {
            std::size_t top = 0;
            std::size_t right = 0;
            std::size_t bottom = 0;
            std::size_t left = 0;
        };

        [[nodiscard]] RingPlan ringPlan(std::size_t seats) noexcept
        {
            const auto side = seats / 4;
            const auto rest = seats - (2 * side);

            return RingPlan{
                .top = (rest + 1) / 2,
                .right = side,
                .bottom = rest / 2,
                .left = side};
        }

        void describeStatus(
            Context &ui, const SeatSnapshot &seat, bool handInProgress)
        {
            if (seat.roundCommitted > 0)
            {
                ui.label(
                    "bet " + std::to_string(seat.roundCommitted),
                    kInFront);

                return;
            }

            if (seat.inHand && seat.stack == 0)
            {
                ui.label("all in", kInFront);

                return;
            }

            if (handInProgress && !seat.inHand)
            {
                ui.label("folded", kDim);

                return;
            }

            if (seat.isToAct)
            {
                ui.label("to act", kToAct);

                return;
            }

            ui.label("waiting", kDim);
        }
    }

    void TableScene::draw(
        IRenderer &renderer,
        Size canvas,
        const TableSnapshot &snapshot,
        OptionalAtlas atlas) const
    {
        renderer.clear(kFelt);

        const auto frame = describe(canvas, snapshot);

        if (atlas.has_value())
        {
            for (const auto &blit :
                 describeArt(canvas, frame.rects, snapshot))
            {
                renderer.drawTexture(
                    atlas->get(),
                    blit.source,
                    blit.destination,
                    blit.tint);
            }
        }

        ui::paint(renderer, frame.commands);
    }

    void TableScene::appendFelt(std::vector<ArtBlit> &art, Size canvas)
    {
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

    void TableScene::appendTable(
        std::vector<ArtBlit> &art, const WidgetRects &rects)
    {
        const auto table = rects.find(widgets::kTable);

        if (!table.has_value())
        {
            return;
        }

        art.push_back(ArtBlit{
            .source = sourceOf(kTableSlot),
            .destination = *table,
            .tint = kWhite});
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

        const auto ink = isRedSuit(card) ? kRedSuit : kBlackSuit;
        const auto width = destination.size.width;
        const auto height = destination.size.height;
        const auto glyph = width / 2;

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
        for (std::size_t index = 0; index < snapshot.board.size();
             ++index)
        {
            const auto face = rects.find(widgets::boardCard(index));

            if (face.has_value())
            {
                appendCard(art, snapshot.board[index], *face, true);
            }
        }

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

        if (!row.has_value())
        {
            return;
        }

        const auto plate = deflated(*row, row->size.height / 8);

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

        appendFelt(art, canvas);
        appendTable(art, rects);
        appendBoard(art, rects, snapshot);
        appendSeats(art, rects, snapshot);

        return art;
    } // GCOVR_EXCL_LINE

    Frame TableScene::describe(
        Size canvas, const TableSnapshot &snapshot) const
    {
        const auto scale = scaleForCanvas(canvas);

        Context ui{canvas, scaledTheme(Theme{}, scale)};

        const auto margin = kGlyphLineHeight * scale;

        {
            const auto table = ui.column(
                {.width = kGrow, .height = kGrow, .gap = 0});

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
                describeRing(ui, snapshot, scale);
            }
        }

        return ui.finish();
    }

    void TableScene::describeHeader(
        Context &ui, const TableSnapshot &snapshot) const
    {
        const auto header = ui.row({.width = kGrow});

        ui.label(headerOf(snapshot), kInk);
        ui.spacer(kGrow);
        ui.label(blindsOf(snapshot), kDim);
    }

    void TableScene::describeTable(
        Context &ui,
        const TableSnapshot &snapshot,
        std::uint32_t scale) const
    {
        const auto board = ui.column(
            {.width = kGrow,
             .height = kGrow,
             .cross = Alignment::Center,
             .padding = scale,
             .gap = scale,
             .id = widgets::kTable});

        ui.spacer(kGrow);

        {
            const auto pot = ui.row(
                {.width = kFit,
                 .height = kFit,
                 .cross = Alignment::Center,
                 .gap = scale});

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

    void TableScene::describeRing(
        Context &ui,
        const TableSnapshot &snapshot,
        std::uint32_t scale) const
    {
        const auto plan = ringPlan(snapshot.seats.size());

        const SeatMetrics metrics{
            .width = seatWidth(scale),
            .height = seatHeight(scale),
            .barRoom = seatWidth(scale) / 2,
            .largestStack = largestStack(snapshot),
            .scale = scale,
            .showButton = snapshot.handsPlayed > 0,
            .handInProgress = snapshot.handInProgress};

        const auto ring =
            ui.column({.width = kGrow, .height = kGrow, .gap = scale});

        {
            const auto top =
                ui.row({.width = kGrow, .height = kFit, .gap = 0});

            describeSeatRun(ui, snapshot, metrics, 0, plan.top, false);
        }

        {
            const auto middle = ui.row(
                {.width = kGrow, .height = kGrow, .gap = scale});

            {
                const auto left = ui.column(
                    {.width = kFit, .height = kGrow, .gap = 0});

                describeSeatRun(
                    ui,
                    snapshot,
                    metrics,
                    plan.top + plan.right + plan.bottom,
                    plan.left,
                    true);
            }

            describeTable(ui, snapshot, scale);

            {
                const auto right = ui.column(
                    {.width = kFit, .height = kGrow, .gap = 0});

                describeSeatRun(
                    ui, snapshot, metrics, plan.top, plan.right, false);
            }
        }

        {
            const auto bottom =
                ui.row({.width = kGrow, .height = kFit, .gap = 0});

            describeSeatRun(
                ui,
                snapshot,
                metrics,
                plan.top + plan.right,
                plan.bottom,
                true);
        }
    }

    void TableScene::describeSeatRun(
        Context &ui,
        const TableSnapshot &snapshot,
        SeatMetrics metrics,
        std::size_t first,
        std::size_t count,
        bool reversed) const
    {
        for (std::size_t step = 0; step < count; ++step)
        {
            ui.spacer(kGrow);

            const auto along = reversed ? count - 1 - step : step;
            metrics.index = first + along;

            describeSeat(ui, snapshot.seats[metrics.index], metrics);
        }

        ui.spacer(kGrow);
    }

    void TableScene::describeSeat(
        Context &ui, const SeatSnapshot &seat, SeatMetrics metrics) const
    {
        const auto border = ui.column(
            {.width = fixedSize(metrics.width),
             .height = fixedSize(metrics.height),
             .background = seat.isToAct ? kToAct : kRail,
             .padding = metrics.scale,
             .gap = 0,
             .id = widgets::seat(metrics.index)});

        const auto boxPadding = 2 * metrics.scale;

        const auto box = ui.column(
            {.width = kGrow,
             .height = kGrow,
             .cross = Alignment::Center,
             .background = kSeatBox,
             .padding = boxPadding,
             .gap = 0});

        if (!seat.occupied)
        {
            ui.label("-- empty --", kDim);

            return;
        }

        {
            const auto head = ui.row(
                {.width = kGrow, .height = kFit, .gap = metrics.scale});

            {
                const auto details = ui.column(
                    {.width = kGrow, .height = kFit, .gap = 0});

                auto label = seat.name;
                if (seat.isButton && metrics.showButton)
                {
                    label += " (D)";
                }

                ui.label(label, seat.inHand ? kInk : kDim);
                ui.label(std::to_string(seat.stack), kInk);

                describeStatus(ui, seat, metrics.handInProgress);

                {
                    const auto bar = ui.row(
                        {.width = fixedSize(static_cast<std::uint32_t>(
                             seat.stack * metrics.barRoom
                             / metrics.largestStack)),
                         .height = fixedSize(2 * metrics.scale),
                         .background = kStackBar});
                }
            }

            describeBadges(ui, seat, metrics);
        }

        if (seat.inHand)
        {
            describeCards(
                ui,
                seat.holeCards,
                metrics.scale,
                widgets::firstHoleCard(metrics.index));
        }

        ui.spacer(kGrow);
    }

    void TableScene::describeBadges(
        Context &ui, const SeatSnapshot &seat, SeatMetrics metrics) const
    {
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

}
