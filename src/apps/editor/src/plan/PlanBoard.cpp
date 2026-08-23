#include "antwika/editor/plan/PlanBoard.hpp"

#include <algorithm>

#include <antwika/enums/Enumeration.hpp>

namespace antwika::editor
{

    std::string_view columnName(const Column whichColumn)
    {
        switch (whichColumn)
        {
        case Column::Todo:
            return "To do";
        case Column::Doing:
            return "Doing";
        case Column::Done:
            return "Done";
        }

        return "";
    }

    const std::vector<Card> &cardsOf(
        const Board &board, const Column whichColumn)
    {
        return board.columnCards.at(enums::index(whichColumn));
    }

    bool addCard(Board &board, const Column whichColumn, Card card)
    {
        auto &columnCards = board.columnCards.at(enums::index(whichColumn));

        if (columnCards.size() >= kMaxCardsPerColumn)
        {
            return false;
        }

        columnCards.push_back(std::move(card));

        return true;
    }

    bool removeCard(
        Board &board, const Column whichColumn, const std::size_t cardIndex)
    {
        auto &columnCards = board.columnCards.at(enums::index(whichColumn));

        if (cardIndex >= columnCards.size())
        {
            return false;
        }

        columnCards.erase(
            columnCards.begin() + static_cast<std::ptrdiff_t>(cardIndex));

        return true;
    }

    std::optional<std::size_t> moveCard(
        Board &board,
        const Column fromColumn,
        const std::size_t cardIndex,
        const Column toColumn,
        const std::size_t dropIndex)
    {
        auto &leaving = board.columnCards.at(enums::index(fromColumn));

        if (cardIndex >= leaving.size())
        {
            return std::nullopt;
        }

        const auto sameColumn = fromColumn == toColumn;

        if (!sameColumn
            && board.columnCards.at(enums::index(toColumn)).size()
                   >= kMaxCardsPerColumn)
        {
            return std::nullopt;
        }

        auto landing = std::min(
            dropIndex,
            sameColumn ? leaving.size()
                       : board.columnCards.at(
                           enums::index(toColumn)).size());

        if (sameColumn && landing > cardIndex)
        {
            --landing;
        }

        if (sameColumn && landing == cardIndex)
        {
            return std::nullopt;
        }

        auto carriedCard = std::move(leaving.at(cardIndex));

        leaving.erase(
            leaving.begin() + static_cast<std::ptrdiff_t>(cardIndex));

        auto &arriving = board.columnCards.at(enums::index(toColumn));

        arriving.insert(
            arriving.begin() + static_cast<std::ptrdiff_t>(landing),
            std::move(carriedCard));

        return landing;
    }

    std::size_t dropIndex(
        const std::span<const gfx::Rect> cardRects,
        const std::int32_t pointerY)
    {
        std::size_t landing = 0;

        for (const auto &card : cardRects)
        {
            const auto middle =
                card.originPoint.y
                + static_cast<std::int32_t>(card.size.height / 2);

            if (pointerY < middle)
            {
                break;
            }

            ++landing;
        }

        return landing;
    }

    std::optional<std::pair<Column, std::size_t>> cardOfWidget(
        const widget::WidgetId whichWidget)
    {
        const auto idValue = static_cast<std::uint64_t>(whichWidget);
        const auto span = kEveryColumn.size() * kMaxCardsPerColumn;

        if (idValue < kFirstPlanCardWidget
            || idValue >= kFirstPlanCardWidget + span)
        {
            return std::nullopt;
        }

        const auto rank = idValue - kFirstPlanCardWidget;

        return std::pair{
            kEveryColumn.at(rank / kMaxCardsPerColumn),
            rank % kMaxCardsPerColumn};
    }

}
