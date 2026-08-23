#include "antwika/editor/ui/PlanView.hpp"

#include <cmath>
#include <cstdlib>

#include <antwika/editor/ui/AtlasView.hpp>
#include <antwika/editor/ui/EditorLook.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/text/TextLayout.hpp>
#include <antwika/ui/TextWrap.hpp>

#include "antwika/editor/plan/PlanFile.hpp"
#include "antwika/editor/plan/PlanFileError.hpp"

namespace antwika::editor
{

    namespace
    {
        constexpr std::int32_t kCarryDeadZone = 4;

        constexpr std::uint32_t kDetailWidth = kPickerWidth * kUiScale;

        constexpr std::uint32_t kDropMarkerHeight = 2 * kUiScale;

        constexpr std::string_view kUntitled = "(untitled)";

        [[nodiscard]] bool covers(
            const antwika::gfx::Rect &roomRect,
            const antwika::gfx::Point &point)
        {
            return point.x >= roomRect.originPoint.x
                   && point.y >= roomRect.originPoint.y
                   && point.x < roomRect.originPoint.x
                          + static_cast<std::int32_t>(roomRect.size.width)
                   && point.y < roomRect.originPoint.y
                          + static_cast<std::int32_t>(roomRect.size.height);
        }
    }

    void PlanView::open(std::string path)
    {
        planPath = std::move(path);
        planBoard = loadBoard(planPath).value_or(Board{});
    }

    const Board &PlanView::board() const noexcept
    {
        return planBoard;
    }

    bool PlanView::unsaved() const noexcept
    {
        return planUnsaved;
    }

    bool PlanView::dragging() const noexcept
    {
        return planDrag.has_value() && planDrag->moved;
    }

    bool PlanView::picked() const noexcept
    {
        return planPicked.has_value();
    }

    void PlanView::carry(PlanDrag heldDrag)
    {
        planDrag = heldDrag;
    }

    void PlanView::draggedTo(const gfx::Point pointer)
    {
        if (!planDrag.has_value() || planDrag->moved)
        {
            return;
        }

        planDrag->moved =
            std::abs(pointer.x - planDrag->grabbedAtPoint.x)
                + std::abs(pointer.y - planDrag->grabbedAtPoint.y)
            > kCarryDeadZone;
    }

    std::optional<std::string> PlanView::letGo()
    {
        std::optional<std::string> notice;

        if (!planDrag.has_value())
        {
            planBodyDrag = antwika::ui::DragOrigin::None;

            return notice;
        }

        if (planDrag->moved && planDrag->overColumn.has_value())
        {
            const auto movedBoard = moveCard(
                planBoard,
                planDrag->fromColumn,
                planDrag->cardIndex,
                *planDrag->overColumn,
                planDrag->dropIndex);

            if (movedBoard.has_value())
            {
                planPicked = std::pair{*planDrag->overColumn, *movedBoard};
                planUnsaved = true;
            }
            else if (
                *planDrag->overColumn != planDrag->fromColumn
                && cardsOf(planBoard, *planDrag->overColumn).size()
                       >= kMaxCardsPerColumn)
            {
                notice = "that column is full";
            }
        }

        planDrag.reset();
        planBodyDrag = antwika::ui::DragOrigin::None;

        return notice;
    }

    void PlanView::endDrag() noexcept
    {
        planDrag.reset();
    }

    std::optional<std::uint32_t> PlanView::cardWidth() const
    {
        if (planColumnWidth <= kPanelPadding * 2)
        {
            return std::nullopt;
        }

        return planColumnWidth - (kPanelPadding * 2);
    }

    void PlanView::layout(
        ui::Context &context, const FocusedField focusedField)
    {
        const auto rowWidget = context.row(
            antwika::ui::ContainerSpec{
                .widthSizing = antwika::ui::kGrowSizing,
                .heightSizing = antwika::ui::kGrowSizing,
                .padding = kPanelPadding,
                .gap = kPanelGap * kUiScale});

        for (const auto which : kEveryColumn)
        {
            const auto &columnCards = cardsOf(planBoard, which);
            const auto carrying =
                planDrag.has_value() && planDrag->moved;
            const auto marking =
                carrying
                && (planDrag->overColumn.has_value()
                        ? *planDrag->overColumn == which
                        : planDrag->fromColumn == which);
            const std::size_t markedIndex =
                !carrying                    ? 0
                : planDrag->overColumn.has_value() ? planDrag->dropIndex
                                             : planDrag->cardIndex;

            const auto list = context.column(
                antwika::ui::ContainerSpec{
                    .widthSizing = antwika::ui::kGrowSizing,
                    .heightSizing = antwika::ui::kGrowSizing,
                    .backgroundColor = kPanelColor,
                    .padding = kPanelPadding,
                    .widgetId = planColumnWidget(which),
                    .clips = true});

            panelTitle(context, std::string(columnName(which)));

            for (std::size_t index = 0; index < columnCards.size(); ++index)
            {
                if (marking && index == markedIndex)
                {
                    const auto slot = context.column(
                        antwika::ui::ContainerSpec{
                            .widthSizing = antwika::ui::kGrowSizing,
                            .heightSizing = antwika::ui::fixedSize(
                                kDropMarkerHeight),
                            .backgroundColor = kSelectionAccentColor});
                }

                const auto cardPicked =
                    planPicked.has_value()
                    && planPicked->first == which
                    && planPicked->second == index;
                const auto carryingHere =
                    carrying && planDrag->fromColumn == which
                    && planDrag->cardIndex == index;
                const auto &card = columnCards.at(index);

                context.button(
                    card.title.empty() ? std::string(kUntitled)
                                       : card.title,
                    antwika::ui::ButtonSpec{
                        .widgetId = planCardWidget(which, index),
                        .widthSizing = antwika::ui::kGrowSizing,
                        .state =
                            cardPicked
                                ? std::optional{
                                      antwika::ui::ButtonState::
                                          Pressed}
                                : std::nullopt,
                        .fillColor = carryingHere
                                   ? std::optional{kTitleBarColor}
                                   : std::nullopt,
                        .wrapWidth = cardWidth()});
            }

            if (marking && markedIndex >= columnCards.size())
            {
                const auto slot = context.column(
                    antwika::ui::ContainerSpec{
                        .widthSizing = antwika::ui::kGrowSizing,
                        .heightSizing =
                            antwika::ui::fixedSize(kDropMarkerHeight),
                        .backgroundColor = kSelectionAccentColor});
            }

            context.spacer(antwika::ui::kGrowSizing);

            if (columnCards.size() < kMaxCardsPerColumn)
            {
                context.button(
                    "+ card",
                    antwika::ui::ButtonSpec{
                        .widgetId = planAddWidget(which),
                        .widthSizing = antwika::ui::kGrowSizing});
            }
        }

        const auto pane = context.column(
            antwika::ui::ContainerSpec{
                .widthSizing = antwika::ui::fixedSize(kDetailWidth),
                .heightSizing = antwika::ui::kGrowSizing,
                .backgroundColor = kPanelColor,
                .padding = kPanelPadding,
                .gap = kUiScale * 2,
                .widgetId = kPlanDetailWidget});

        panelTitle(context, "Card");

        if (!planPicked.has_value())
        {
            context.label("Pick a card to write in it.", kGridLineColor);

            return;
        }

        const auto &card =
            cardsOf(planBoard, planPicked->first).at(planPicked->second);

        context.textField(
            antwika::ui::TextFieldSpec{
                .widgetId = kPlanTitleWidget,
                .text = card.title,
                .placeholder = "title",
                .focused = focusedField == FocusedField::PlanTitle});
        context.textArea(
            antwika::ui::TextAreaSpec{
                .widgetId = kPlanBodyWidget,
                .text = card.body,
                .placeholder = "what it means",
                .cursor = planBodyCursor,
                .anchor = planBodyAnchor,
                .scroll = planBodyScroll,
                .scrollbar = true,
                .focused = focusedField == FocusedField::PlanBody,
                .dragging = planBodyDrag});
        context.button(
            "Delete card",
            antwika::ui::ButtonSpec{
                .widgetId = kPlanDeleteWidget,
                .widthSizing = antwika::ui::kGrowSizing});
    }

    void PlanView::updateFrame(
        const ui::Frame &frame,
        const std::optional<gfx::Point> pointerInWindow)
    {
        const auto first =
            frame.rects.find(planColumnWidget(kEveryColumn.front()));

        if (first.has_value())
        {
            planColumnWidth = first->size.width;
        }

        if (!planDrag.has_value() || !planDrag->moved
            || !pointerInWindow.has_value())
        {
            return;
        }

        planDrag->overColumn.reset();

        for (const auto which : kEveryColumn)
        {
            const auto room = frame.rects.find(planColumnWidget(which));

            if (!room.has_value()
                || !covers(*room, *pointerInWindow))
            {
                continue;
            }

            std::vector<antwika::gfx::Rect> drawnRects;

            for (std::size_t index = 0;
                 index < cardsOf(planBoard, which).size();
                 ++index)
            {
                const auto card =
                    frame.rects.find(planCardWidget(which, index));

                if (card.has_value())
                {
                    drawnRects.push_back(*card);
                }
            }

            planDrag->overColumn = which;
            planDrag->dropIndex =
                dropIndex(drawnRects, pointerInWindow->y);
        }
    }

    void PlanView::drawGhost(
        gfx::ViewportRenderer &viewportRenderer,
        const std::optional<gfx::Point> pointerInWindow)
    {
        if (!planDrag.has_value() || !planDrag->moved
            || !pointerInWindow.has_value())
        {
            return;
        }

        const auto &columnCards = cardsOf(planBoard, planDrag->fromColumn);

        if (planDrag->cardIndex >= columnCards.size())
        {
            return;
        }

        const auto &card = columnCards.at(planDrag->cardIndex);
        const auto titleText = card.title.empty()
                             ? std::string(kUntitled)
                             : card.title;
        const auto room = cardWidth();
        const auto lines = antwika::ui::wrapText(
            titleText,
            room.has_value()
                ? antwika::ui::wrapColumns(gameTheme(), *room)
                : 0);
        const auto pad = static_cast<float>(2 * kUiScale);
        const auto step = text::textSize(titleText, kUiScale).height;

        std::uint32_t widest = 0;

        for (const auto line : lines)
        {
            widest = std::max(
                widest, text::textSize(line, kUiScale).width);
        }

        const auto width =
            static_cast<float>(widest) + (pad * 2.0F);
        const auto height =
            static_cast<float>(
                step * static_cast<std::uint32_t>(lines.size()))
            + (pad * 2.0F);
        const auto left =
            static_cast<float>(pointerInWindow->x) - (width / 2.0F);
        const auto top =
            static_cast<float>(pointerInWindow->y) - (height / 2.0F);

        auto &nativeRenderer = viewportRenderer.nativeRenderer();

        nativeRenderer.drawRect(
            antwika::gfx::RectF(
                antwika::gfx::PointF{left, top},
                antwika::gfx::SizeF{width, height}),
            kTitleBarColor);

        for (std::size_t index = 0; index < lines.size(); ++index)
        {
            nativeRenderer.drawText(
                antwika::gfx::PointF{
                    left + pad,
                    top + pad
                        + static_cast<float>(
                            step * static_cast<std::uint32_t>(index))},
                lines.at(index),
                kUiScale,
                kSelectionAccentColor);
        }
    }

    bool PlanView::handleWidgets(
        const ui::Interactions &interactions,
        const std::optional<gfx::Point> pointerInWindow,
        FocusedField &focusedField,
        std::optional<std::string> &notice)
    {

        for (const auto which : kEveryColumn)
        {
            if (interactions.activatedWidget != planAddWidget(which))
            {
                continue;
            }

            if (!addCard(planBoard, which, Card{}))
            {
                notice = "that column is full";

                return true;
            }

            planPicked =
                std::pair{which, cardsOf(planBoard, which).size() - 1};
            focusedField = FocusedField::PlanTitle;
            planBodyCursor = 0;
            planBodyAnchor = 0;
            planBodyScroll = 0;
            planUnsaved = true;

            return true;
        }

        if (interactions.activatedWidget == kPlanDeleteWidget
            && planPicked.has_value())
        {
            if (removeCard(planBoard, planPicked->first, planPicked->second))
            {
                notice = "card deleted";
                planUnsaved = true;
            }

            planPicked.reset();
            focusedField = FocusedField::Nothing;

            return true;
        }

        if (interactions.activatedWidget == kPlanTitleWidget
            && planPicked.has_value())
        {
            focusedField = FocusedField::PlanTitle;

            return true;
        }

        if (interactions.activatedWidget == kPlanBodyWidget
            && planPicked.has_value())
        {
            focusedField = FocusedField::PlanBody;

            return true;
        }

        const auto card = cardOfWidget(interactions.activatedWidget);

        if (!card.has_value() || !pointerInWindow.has_value()
            || card->second >= cardsOf(planBoard, card->first).size())
        {
            return false;
        }

        planPicked = *card;
        focusedField = FocusedField::Nothing;
        planBodyCursor = 0;
        planBodyAnchor = 0;
        planBodyScroll = 0;
        planDrag = PlanDrag{
            .fromColumn = card->first,
            .cardIndex = card->second,
            .grabbedAtPoint = *pointerInWindow,
            .moved = false,
            .overColumn = std::nullopt,
            .dropIndex = 0};

        return true;
    }

    void PlanView::carryEdits(
        const ui::Frame &frame, FocusedField &focusedField)
    {
        if (!planPicked.has_value())
        {
            return;
        }

        auto &columnCards = planBoard.columnCards.at(
            static_cast<std::size_t>(planPicked->first));

        if (planPicked->second >= columnCards.size())
        {
            planPicked.reset();

            return;
        }

        auto &card = columnCards.at(planPicked->second);
        const auto &interactions = frame.interactions;

        if (interactions.edit.has_value()
            && interactions.edit->fieldWidget == kPlanTitleWidget)
        {
            card.title = interactions.edit->text;
            planUnsaved = true;

            if (interactions.edit->submitted
                || interactions.edit->cancelled)
            {
                focusedField = FocusedField::Nothing;
            }
        }

        if (interactions.edit.has_value()
            && interactions.edit->fieldWidget == kPlanBodyWidget)
        {
            card.body = interactions.edit->text;
            planBodyCursor = interactions.edit->cursor;
            planBodyAnchor = interactions.edit->anchor;
            planUnsaved = true;

            if (interactions.edit->cancelled)
            {
                focusedField = FocusedField::Nothing;
            }
        }

        if (interactions.scrollChange.has_value()
            && interactions.scrollChange->areaWidget == kPlanBodyWidget)
        {
            planBodyScroll = interactions.scrollChange->line;
        }

        if (interactions.areaPress.has_value()
            && interactions.areaPress->areaWidget == kPlanBodyWidget)
        {
            planBodyDrag = interactions.areaPress->homeOrigin;
        }
    }

    std::optional<std::string> PlanView::save()
    {
        if (!planUnsaved)
        {
            return std::nullopt;
        }

        try
        {
            saveBoard(planPath, planBoard);
            planUnsaved = false;
        }
        catch (const PlanFileError &error)
        {
            return std::string(error.what());
        }

        return std::nullopt;
    }

}
