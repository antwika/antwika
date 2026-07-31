#pragma once

#include <cstddef>
#include <cstdint>

#include <antwika/ui/WidgetId.hpp>

namespace antwika::poker::widgets
{

    using antwika::ui::WidgetId;

    /**
     * @brief Where a table's art is allowed to land.
     *
     * Every one of these names a container TableScene::describe() lays
     * out and TableScene::describeArt() then blits into.
     * They exist so the art has somewhere to read its rectangles from:
     * a card, a chip or a plate is placed *by* the layout rather than
     * beside it, since two independently computed layouts agree only
     * until either one of them changes.
     *
     * Nothing here is pointed at. The table is watched rather than
     * played with, so an id is a name for a rectangle and nothing more;
     * naming a container would also make it hoverable, and this frame is
     * handed no pointer at all.
     *
     * The blocks are spaced out rather than packed, so that adding a
     * seat or a card cannot walk one family's ids into the next one's.
     */

    /**
     * @brief The chip drawn beside the pot.
     */
    inline constexpr WidgetId kPot{1};

    /**
     * @brief The first of the five community cards.
     */
    inline constexpr std::uint64_t kFirstBoardCard = 0x100;

    /**
     * @brief The first seat row.
     */
    inline constexpr std::uint64_t kFirstSeat = 0x200;

    /**
     * @brief The first seat's dealer badge.
     */
    inline constexpr std::uint64_t kFirstDealerBadge = 0x400;

    /**
     * @brief The first seat's bet badge.
     */
    inline constexpr std::uint64_t kFirstBetBadge = 0x600;

    /**
     * @brief The first hole card of the first seat.
     */
    inline constexpr std::uint64_t kFirstHoleCard = 0x1000;

    /**
     * @brief How many hole cards each seat is given room for.
     *
     * More than hold'em deals, so that a seat's block never runs into
     * the next seat's.
     */
    inline constexpr std::uint64_t kHoleCardsPerSeat = 8;

    /**
     * @brief Name one community card.
     * @param index Which board card, counting from the flop.
     * @return Its id.
     */
    [[nodiscard]] constexpr WidgetId boardCard(std::size_t index) noexcept
    {
        return static_cast<WidgetId>(
            kFirstBoardCard + static_cast<std::uint64_t>(index));
    }

    /**
     * @brief Name one seat's row.
     * @param index Which seat, in the order the table holds them.
     * @return Its id.
     */
    [[nodiscard]] constexpr WidgetId seat(std::size_t index) noexcept
    {
        return static_cast<WidgetId>(
            kFirstSeat + static_cast<std::uint64_t>(index));
    }

    /**
     * @brief Name where one seat's dealer button goes.
     * @param index Which seat.
     * @return Its id.
     */
    [[nodiscard]] constexpr WidgetId dealerBadge(
        std::size_t index) noexcept
    {
        return static_cast<WidgetId>(
            kFirstDealerBadge + static_cast<std::uint64_t>(index));
    }

    /**
     * @brief Name where one seat's bet marker goes.
     * @param index Which seat.
     * @return Its id.
     */
    [[nodiscard]] constexpr WidgetId betBadge(std::size_t index) noexcept
    {
        return static_cast<WidgetId>(
            kFirstBetBadge + static_cast<std::uint64_t>(index));
    }

    /**
     * @brief Name one seat's first hole card.
     *
     * The rest of that seat's cards follow it, which is what after()
     * counts along.
     *
     * @param index Which seat.
     * @return The id of that seat's first card.
     */
    [[nodiscard]] constexpr WidgetId firstHoleCard(
        std::size_t index) noexcept
    {
        return static_cast<WidgetId>(
            kFirstHoleCard
            + (static_cast<std::uint64_t>(index) * kHoleCardsPerSeat));
    }

    /**
     * @brief Step along a run of cards from the first of them.
     *
     * A row of cards is described by one loop whether it is the board
     * or somebody's hand, so it counts from whichever id it was given
     * rather than knowing which family that was.
     *
     * @param first The id of the first card in the run.
     * @param index How far along the run to step.
     * @return The id of that card.
     */
    [[nodiscard]] constexpr WidgetId after(
        WidgetId first, std::size_t index) noexcept
    {
        return static_cast<WidgetId>(
            static_cast<std::uint64_t>(first)
            + static_cast<std::uint64_t>(index));
    }

} // namespace antwika::poker::widgets
