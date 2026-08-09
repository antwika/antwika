#pragma once

#include <cstddef>
#include <cstdint>

#include <antwika/ui/WidgetId.hpp>

namespace antwika::poker::widgets
{

    using antwika::ui::WidgetId;

    inline constexpr WidgetId kPot{1};

    inline constexpr WidgetId kTable{2};

    inline constexpr std::uint64_t kFirstBoardCard = 0x100;

    inline constexpr std::uint64_t kFirstSeat = 0x200;

    inline constexpr std::uint64_t kFirstDealerBadge = 0x400;

    inline constexpr std::uint64_t kFirstBetBadge = 0x600;

    inline constexpr std::uint64_t kFirstHoleCard = 0x1000;

    inline constexpr std::uint64_t kHoleCardsPerSeat = 8;

    [[nodiscard]] constexpr WidgetId boardCard(std::size_t index) noexcept
    {
        return static_cast<WidgetId>(
            kFirstBoardCard + static_cast<std::uint64_t>(index));
    }

    [[nodiscard]] constexpr WidgetId seat(std::size_t index) noexcept
    {
        return static_cast<WidgetId>(
            kFirstSeat + static_cast<std::uint64_t>(index));
    }

    [[nodiscard]] constexpr WidgetId dealerBadge(
        std::size_t index) noexcept
    {
        return static_cast<WidgetId>(
            kFirstDealerBadge + static_cast<std::uint64_t>(index));
    }

    [[nodiscard]] constexpr WidgetId betBadge(std::size_t index) noexcept
    {
        return static_cast<WidgetId>(
            kFirstBetBadge + static_cast<std::uint64_t>(index));
    }

    [[nodiscard]] constexpr WidgetId firstHoleCard(
        std::size_t index) noexcept
    {
        return static_cast<WidgetId>(
            kFirstHoleCard
            + (static_cast<std::uint64_t>(index) * kHoleCardsPerSeat));
    }

    [[nodiscard]] constexpr WidgetId after(
        WidgetId first, std::size_t index) noexcept
    {
        return static_cast<WidgetId>(
            static_cast<std::uint64_t>(first)
            + static_cast<std::uint64_t>(index));
    }

}
