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

    [[nodiscard]] std::optional<std::pair<Column, std::size_t>>
    getCardOfWidget(widget::WidgetId whichWidget);

}
