#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <antwika/gfx/Rect.hpp>
#include <antwika/widget/WidgetId.hpp>

#include "antwika/editor/plan/Board.hpp"
#include "antwika/editor/plan/Card.hpp"

namespace antwika::editor
{

    enum class Column : std::uint8_t
    {
        Todo,
        Doing,
        Done,
    };

    [[nodiscard]] constexpr Column getLastEnumerator(Column) noexcept
    {
        return Column::Done;
    }

    inline constexpr std::array<Column, 3> kEveryColumn{
        Column::Todo, Column::Doing, Column::Done};

    inline constexpr std::size_t kMaxCardsPerColumn = 64;

    [[nodiscard]] std::string_view getColumnName(Column whichColumn);

    [[nodiscard]] const std::vector<Card> &cardsOf(
        const Board &board, Column whichColumn);

    [[nodiscard]] bool addCard(Board &board, Column whichColumn, Card card);

    [[nodiscard]] bool removeCard(
        Board &board, Column whichColumn, std::size_t cardIndex);

    [[nodiscard]] std::optional<std::size_t> moveCard(
        Board &board,
        Column fromColumn,
        std::size_t cardIndex,
        Column toColumn,
        std::size_t dropIndex);

    [[nodiscard]] std::size_t getDropIndex(
        std::span<const gfx::Rect> cardRects, std::int32_t pointerY);

    inline constexpr widget::WidgetId kPlanDetailWidget{410};

    inline constexpr widget::WidgetId kPlanTitleWidget{411};

    inline constexpr widget::WidgetId kPlanBodyWidget{412};

    inline constexpr widget::WidgetId kPlanDeleteWidget{413};

    inline constexpr std::uint64_t kFirstPlanAddWidget = 416;

    inline constexpr std::uint64_t kFirstPlanColumnWidget = 420;

    inline constexpr std::uint64_t kFirstPlanCardWidget = 512;

    [[nodiscard]] constexpr widget::WidgetId getPlanColumnWidget(
        const Column whichColumn) noexcept
    {
        return widget::WidgetId{
            kFirstPlanColumnWidget
            + static_cast<std::uint64_t>(whichColumn)};
    }

    [[nodiscard]] constexpr widget::WidgetId getPlanAddWidget(
        const Column whichColumn) noexcept
    {
        return widget::WidgetId{
            kFirstPlanAddWidget
            + static_cast<std::uint64_t>(whichColumn)};
    }

    [[nodiscard]] constexpr widget::WidgetId getPlanCardWidget(
        const Column whichColumn, const std::size_t cardIndex) noexcept
    {
        return widget::WidgetId{
            kFirstPlanCardWidget
            + (static_cast<std::uint64_t>(whichColumn)
               * kMaxCardsPerColumn)
            + cardIndex};
    }

    [[nodiscard]] std::optional<std::pair<Column, std::size_t>>
    getCardOfWidget(widget::WidgetId whichWidget);

}
