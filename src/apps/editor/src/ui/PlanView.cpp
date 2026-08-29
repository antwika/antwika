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

#include "antwika/editor/ui/WidgetIds.hpp"

namespace antwika::editor
{

    namespace
    {
        constexpr std::int32_t kCarryDeadZone = 4;

        constexpr std::uint32_t kDetailWidth = kPickerWidth * kUiScale;

        constexpr std::uint32_t kDropMarkerHeight = 2 * kUiScale;

        constexpr std::string_view kUntitled = "(untitled)";

        void dropMarker(ui::Context &context)
        {
            const auto slot = context.column(
                antwika::ui::ContainerSpec{
                    .widthSizing = antwika::ui::kGrowSizing,
                    .heightSizing =
                        antwika::ui::getFixedSize(kDropMarkerHeight),
                    .backgroundColor = kSelectionAccentColor});
        }

        [[nodiscard]] antwika::ui::Sizing getColumnSizing(
            const Column whichColumn, const PanelDrag &panelDrag)
        {
            const auto wish =
                whichColumn == Column::Todo
                    ? panelDrag.panelSizes.planFirstWidth
                : whichColumn == Column::Doing
                    ? panelDrag.panelSizes.planSecondWidth
                    : 0U;

            return wish == 0
                       ? antwika::ui::kGrowSizing
                       : antwika::ui::getFixedSize(
                             getFittedPanelWidth(
                                 wish, wish, panelDrag.windowSize.width));
        }

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
        planBoard = getLoadBoard(planPath).value_or(Board{});
    }

    const Board &PlanView::getBoard() const noexcept
    {
        return planBoard;
    }

    bool PlanView::isUnsaved() const noexcept
    {
        return planUnsaved;
    }

    bool PlanView::isDragging() const noexcept
    {
        return planDrag.has_value() && planDrag->moved;
    }

    bool PlanView::isPicked() const noexcept
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

    std::optional<std::uint32_t> PlanView::getCardWidth() const
    {
        if (planColumnWidth <= kPanelPadding * 2)
        {
            return std::nullopt;
        }

        return planColumnWidth - (kPanelPadding * 2);
    }

    void PlanView::layout(
        ui::Context &context,
        const FocusedField focusedField,
        const PanelDrag &panelDrag)
    {
        const auto rowWidget = context.row(
            antwika::ui::ContainerSpec{
                .widthSizing = antwika::ui::kGrowSizing,
                .heightSizing = antwika::ui::kGrowSizing,
                .padding = kPanelPadding,
                .gap = kPanelGap * kUiScale});

        for (const auto which : kEveryColumn)
        {
            layoutColumn(context, which, panelDrag);

            const auto edgeWidget = getPlanEdgeWidget(which);

            if (edgeWidget != widget::kNoWidget)
            {
                context.edge(
                    antwika::ui::EdgeSpec{
                        .widgetId = edgeWidget,
                        .panelWidget = getPlanColumnWidget(which),
                        .minimum = kMinPanelWidth,
                        .maximum = panelDrag.windowSize.width / 3,
                        .dragging =
                            panelDrag.heldEdgeWidget == edgeWidget});
            }
        }

        context.edge(
            antwika::ui::EdgeSpec{
                .widgetId = kPlanDetailEdgeWidget,
                .panelWidget = kPlanDetailWidget,
                .minimum = kMinPanelWidth,
                .maximum = panelDrag.windowSize.width / 3,
                .dragging =
                    panelDrag.heldEdgeWidget == kPlanDetailEdgeWidget});

        layoutCardPane(context, focusedField, panelDrag);
    }

    void PlanView::layoutColumn(
        ui::Context &context,
        const Column whichColumn,
        const PanelDrag &panelDrag)
    {
        const auto &columnCards = cardsOf(planBoard, whichColumn);
        const auto carrying =
            planDrag.has_value() && planDrag->moved;
        const auto marking =
            carrying
            && (planDrag->overColumn.has_value()
                    ? *planDrag->overColumn == whichColumn
                    : planDrag->fromColumn == whichColumn);
        const std::size_t markedIndex =
            !carrying                    ? 0
            : planDrag->overColumn.has_value() ? planDrag->dropIndex
                                         : planDrag->cardIndex;

        const auto list = context.column(
            antwika::ui::ContainerSpec{
                .widthSizing = getColumnSizing(whichColumn, panelDrag),
                .heightSizing = antwika::ui::kGrowSizing,
                .backgroundColor = kPanelColor,
                .padding = kPanelPadding,
                .widgetId = getPlanColumnWidget(whichColumn),
                .clips = true});

        panelTitle(context, std::string(getColumnName(whichColumn)));

        for (std::size_t index = 0; index < columnCards.size(); ++index)
        {
            if (marking && index == markedIndex)
            {
                dropMarker(context);
            }

            const auto cardPicked =
                planPicked.has_value()
                && planPicked->first == whichColumn
                && planPicked->second == index;
            const auto carryingHere =
                carrying && planDrag->fromColumn == whichColumn
                && planDrag->cardIndex == index;
            const auto &card = columnCards.at(index);

            context.button(
                card.title.empty() ? std::string(kUntitled)
                                   : card.title,
                antwika::ui::ButtonSpec{
                    .widgetId = getPlanCardWidget(whichColumn, index),
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
                    .wrapWidth = getCardWidth()});
        }

        if (marking && markedIndex >= columnCards.size())
        {
            dropMarker(context);
        }

        context.spacer(antwika::ui::kGrowSizing);

        if (columnCards.size() < kMaxCardsPerColumn)
        {
            context.button(
                "+ card",
                antwika::ui::ButtonSpec{
                    .widgetId = getPlanAddWidget(whichColumn),
                    .widthSizing = antwika::ui::kGrowSizing});
        }
    }

    void PlanView::layoutCardPane(
        ui::Context &context,
        const FocusedField focusedField,
        const PanelDrag &panelDrag)
    {
        const auto pane = context.column(
            antwika::ui::ContainerSpec{
                .widthSizing = antwika::ui::getFixedSize(
                    getFittedPanelWidth(
                        panelDrag.panelSizes.cardWidth,
                        kDetailWidth,
                        panelDrag.windowSize.width)),
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
            frame.rects.getWidgetRect(getPlanColumnWidget(kEveryColumn.front()));

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
            const auto room = frame.rects.getWidgetRect(getPlanColumnWidget(which));

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
                    frame.rects.getWidgetRect(getPlanCardWidget(which, index));

                if (card.has_value())
                {
                    drawnRects.push_back(*card);
                }
            }

            planDrag->overColumn = which;
            planDrag->dropIndex =
                getDropIndex(drawnRects, pointerInWindow->y);
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
        const auto room = getCardWidth();
        const auto lines = antwika::ui::getWrapText(
            titleText,
            room.has_value()
                ? antwika::ui::getWrapColumns(getGameTheme(), *room)
                : 0);
        const auto pad = static_cast<float>(2 * kUiScale);
        const auto step =
            text::getTextSize(
                titleText, antwika::gfx::TextScale{.multiplier = kUiScale})
                .height;

        std::uint32_t widest = 0;

        for (const auto line : lines)
        {
            widest = std::max(
                widest,
                text::getTextSize(
                    line,
                    antwika::gfx::TextScale{.multiplier = kUiScale})
                    .width);
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

        auto &innerRenderer = viewportRenderer.innerRenderer();

        innerRenderer.drawRect(
            antwika::gfx::RectF(
                antwika::gfx::PointF{left, top},
                antwika::gfx::SizeF{width, height}),
            kTitleBarColor);

        for (std::size_t index = 0; index < lines.size(); ++index)
        {
            innerRenderer.drawText(
                antwika::gfx::PointF{
                    left + pad,
                    top + pad
                        + static_cast<float>(
                            step * static_cast<std::uint32_t>(index))},
                lines.at(index),
                antwika::gfx::TextScale{.multiplier = kUiScale},
                kSelectionAccentColor);
        }
    }

    bool PlanView::consumeWidgets(
        const ui::Interactions &interactions,
        const std::optional<gfx::Point> pointerInWindow,
        FocusedField &focusedField,
        std::optional<std::string> &notice)
    {

        for (const auto which : kEveryColumn)
        {
            if (interactions.activatedWidget != getPlanAddWidget(which))
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

        const auto card = getCardOfWidget(interactions.activatedWidget);

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


    bool PlanView::claims(
        const View shownView, const bool playing) const noexcept
    {
        return !playing && shownView == View::Plan;
    }

    std::string PlanView::getStatusText(const ViewContext &) const
    {
        return isPicked() ? "drag a card to carry it - write its "
                            "title and what it means on the right "
                            "- escape leaves a field"
                          : "5 plan - click a card to write in it "
                            "- drag one to carry it between "
                            "columns - ctrl s keeps the board";
    }

    void PlanView::draw(
        const ViewContext &, const ui::Frame &)
    {
    }

    bool PlanView::layoutPanel(
        ui::Context &context, const ViewContext &viewContext)
    {
        layout(
            context,
            viewContext.workbench.focusedField,
            PanelDrag{
                .panelSizes =
                    viewContext.workbench.preferences.panelSizes,
                .heldEdgeWidget =
                    viewContext.workbench.pointer.heldEdgeWidget,
                .windowSize =
                    viewContext.render.viewportRenderer.getWindowSize()});

        return true;
    }

    void PlanView::carryFrame(
        const ui::Frame &frame, const ViewContext &viewContext)
    {
        carryEdits(frame, viewContext.workbench.focusedField);
    }

    void PlanView::drawOverlay(const ViewContext &viewContext)
    {
        drawGhost(
            viewContext.render.viewportRenderer,
            viewContext.workbench.pointer.pointerInWindow);
    }

    bool PlanView::takeWidgets(
        const ui::Interactions &interactions,
        const ViewContext &viewContext,
        std::optional<std::string> &notice)
    {
        return consumeWidgets(
            interactions,
            viewContext.workbench.pointer.pointerInWindow,
            viewContext.workbench.focusedField,
            notice);
    }

    void PlanView::trackPointer(const ViewContext &viewContext)
    {
        const auto &pointer = viewContext.workbench.pointer;

        if (pointer.pointerInWindow.has_value())
        {
            draggedTo(*pointer.pointerInWindow);
        }
    }

    bool PlanView::consumeRelease(
        const ViewContext &viewContext,
        const input::PointerButtonReleased &upReleased)
    {
        if (upReleased.button != input::MouseButton::Left)
        {
            return false;
        }

        if (const auto notice = letGo(); notice.has_value())
        {
            viewContext.notices.showStatus(*notice, true, 120);
        }

        return true;
    }

}
