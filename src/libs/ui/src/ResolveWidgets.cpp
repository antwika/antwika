#include "ResolveWidgets.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include <antwika/gfx/Rect.hpp>

#include "antwika/ui/Axis.hpp"
#include "antwika/ui/EdgeChange.hpp"
#include "antwika/ui/ScrollChange.hpp"
#include "antwika/ui/SliderChange.hpp"
#include "antwika/ui/SplitChange.hpp"
#include "antwika/ui/SplitSpec.hpp"
#include "antwika/ui/TextEdit.hpp"
#include "antwika/ui/WidgetId.hpp"

#include "Area.hpp"
#include "PanelEdge.hpp"
#include "Splitter.hpp"
#include "Contains.hpp"
#include "FocusRing.hpp"
#include "StateColors.hpp"
#include "ScrollBar.hpp"
#include "ScrollPane.hpp"
#include "Saturate.hpp"
#include "TextEditing.hpp"

namespace antwika::ui::detail
{

    namespace
    {
        [[nodiscard]] Color fillFor(
            const StateColors &styleColors, bool under, bool down) noexcept
        {
            if (!under)
            {
                return styleColors.idleColor;
            }

            return down ? styleColors.pressedColor : styleColors.hoveredColor;
        }

        [[nodiscard]] bool isHeldOnRect(
            const Pointer &pointer,
            const bool underOverlay,
            const bool dragging,
            const Rect &rect,
            const bool freeDrag) noexcept
        {
            if (underOverlay || !pointer.positionPoint || !pointer.down)
            {
                return false;
            }

            if (dragging && freeDrag)
            {
                return true;
            }

            if (!pointer.pressed && !dragging)
            {
                return false;
            }

            return contains(rect, *pointer.positionPoint);
        }

        [[nodiscard]] std::size_t indexAt(
            const std::string_view text,
            const std::size_t line,
            const std::size_t column) noexcept
        {
            std::size_t begin = 0;

            for (std::size_t lineIndex = 0; lineIndex < line; ++lineIndex)
            {
                const auto end = getEndOfLine(text, begin);

                if (end == text.size())
                {
                    return text.size();
                }

                begin = end + 1;
            }

            return std::min(begin + column, getEndOfLine(text, begin));
        }

        [[nodiscard]] std::size_t lineOf(
            const std::string_view text, const std::size_t charIndex) noexcept
        {
            return static_cast<std::size_t>(
                std::ranges::count(text.substr(0, charIndex), '\n'));
        }

        [[nodiscard]] std::uint32_t getAcrossOffset(
            std::uint64_t value,
            std::uint64_t range,
            std::uint64_t span) noexcept
        {
            if (range == 0)
            {
                return 0;
            }

            return getClampToU32(std::min(value, range) * span / range);
        }

        [[nodiscard]] std::size_t rowsOf(
            const Area &area, const std::size_t line) noexcept
        {
            std::size_t rows = 1;

            for (const auto &band : area.bandRuns)
            {
                if (band.line == line)
                {
                    rows += band.rows;
                }
            }

            return rows;
        }

        [[nodiscard]] std::size_t getRowsBefore(
            const Area &area, const std::size_t line) noexcept
        {
            std::size_t rows = 0;

            for (std::size_t lineIndex = 0; lineIndex < line; ++lineIndex)
            {
                rows += rowsOf(area, lineIndex);
            }

            return rows;
        }

        struct PageView final
        {
            std::size_t page = 1;

            std::size_t furthest = 0;

            Rect rect{};
        };

        [[nodiscard]] PageView showingOf(
            const LayoutTree &tree, const Area &area) noexcept
        {
            const auto &column = tree.getNode(area.column).arrangedRect;

            const auto page = std::max<std::size_t>(
                column.size.height / area.lineHeight, 1);

            std::size_t furthest = area.lines;
            std::size_t rows = 0;

            while (furthest > 0
                   && rows + rowsOf(area, furthest - 1) <= page)
            {
                rows += rowsOf(area, furthest - 1);
                --furthest;
            }

            furthest = std::min(furthest, area.lines - 1);

            return PageView{
                .page = page,
                .furthest = furthest,
                .rect = column};
        }

        void placeThumb(
            LayoutTree &tree, const Area &area, const PageView &pageView)
        {
            const auto track = tree.getNode(area.track).arrangedRect;

            const auto trackHeight = track.size.height;

            const auto share = getAcrossOffset(
                pageView.page, getRowsBefore(area, area.lines), trackHeight);

            const auto thumb =
                std::clamp(share, std::min(trackHeight, area.lineHeight),
                    trackHeight);

            const auto slack = trackHeight - thumb;

            tree.getNode(area.thumb).arrangedRect = Rect{
                .originPoint =
                    {.x = track.originPoint.x,
                     .y = track.originPoint.y
                          + static_cast<std::int32_t>(getAcrossOffset(
                              getRowsBefore(area, area.scroll),
                              getRowsBefore(area, pageView.furthest),
                              slack))},
                .size = {.width = track.size.width, .height = thumb}};
        }

        [[nodiscard]] std::size_t getLineOnTrack(
            const Rect &trackRect,
            const Point point,
            const std::size_t furthest) noexcept
        {
            const auto downOffset = static_cast<std::uint64_t>(
                point.y - trackRect.originPoint.y);

            return getAcrossOffset(
                downOffset, std::max(trackRect.size.height, 1U) - 1U, furthest);
        }

        void layCaret(
            const Area &area,
            const PageView &pageView,
            const Point point,
            const bool extendsSelection,
            std::optional<TextEdit> &edit)
        {
            const auto downOffset = static_cast<std::uint64_t>(
                static_cast<std::int64_t>(point.y)
                - pageView.rect.originPoint.y);

            const auto right = static_cast<std::uint64_t>(
                static_cast<std::int64_t>(point.x)
                - pageView.rect.originPoint.x);

            auto nextEdit =
                edit.has_value() && edit->fieldWidget == area.widgetId
                                                       ? *edit
                          : TextEdit{ // GCOVR_EXCL_LINE
                      .fieldWidget = area.widgetId,
                      .text = area.text, // GCOVR_EXCL_LINE
                      .cursor = area.cursor,
                      .anchor = area.anchor};

            auto remaining =
                static_cast<std::size_t>(downOffset / area.lineHeight);
            auto line = area.scroll;

            while (line < area.lines
                   && remaining >= rowsOf(area, line))
            {
                remaining -= rowsOf(area, line);
                ++line;
            }

            nextEdit.cursor = indexAt(
                nextEdit.text,
                line,
                static_cast<std::size_t>(right / area.advance));

            if (!extendsSelection)
            {
                nextEdit.anchor = nextEdit.cursor;
            }

            if (!edit.has_value() && nextEdit.cursor == area.cursor
                && nextEdit.anchor == area.anchor)
            {
                return;
            }

            edit = nextEdit;
        }

        [[nodiscard]] std::size_t getFollowedLine(
            const Area &area,
            const PageView &pageView,
            const TextEdit &edit,
            const std::size_t top) noexcept
        {
            const auto line = lineOf(edit.text, edit.cursor);

            std::size_t least = line;
            std::size_t rows = 1;

            while (least > 0
                   && rows + rowsOf(area, least - 1) <= pageView.page)
            {
                rows += rowsOf(area, least - 1);
                --least;
            }

            return std::clamp(top, least, line);
        }

        [[nodiscard]] std::size_t getScrollLine(
            const LayoutTree &tree,
            const Area &area,
            const Pointer &pointer,
            const PageView &pageView,
            const bool onTrack,
            const std::optional<TextEdit> &edit) noexcept
        {
            auto top = std::min(area.scroll, pageView.furthest);

            if (onTrack)
            {
                top = getLineOnTrack(
                    tree.getNode(area.track).arrangedRect,
                    *pointer.positionPoint,
                    pageView.furthest);
            }

            const bool moved = !onTrack && edit.has_value()
                               && edit->fieldWidget == area.widgetId
                               && area.widgetId != kNoWidget;

            return moved ? getFollowedLine(area, pageView, *edit, top) : top;
        }

        void layCaretAtPointer(
            const Area &area,
            const PageView &pageView,
            const Pointer &pointer,
            const bool underOverlay,
            std::optional<TextEdit> &edit)
        {
            const bool inText =
                !underOverlay && area.focused && pointer.positionPoint
                && (pointer.pressed
                    || (pointer.down && pointer.extendsSelection
                        && area.dragging == DragOrigin::Text))
                && contains(pageView.rect, *pointer.positionPoint);

            if (!inText)
            {
                return;
            }

            layCaret(
                area,
                pageView,
                *pointer.positionPoint,
                pointer.extendsSelection,
                edit);
        }

        void noteAreaPress(
            const Area &area,
            const PageView &pageView,
            const Pointer &pointer,
            const bool underOverlay,
            const bool onTrack,
            Interactions &interactions)
        {
            if (underOverlay || !pointer.pressed || !pointer.positionPoint
                || area.widgetId == kNoWidget)
            {
                return;
            }

            if (onTrack)
            {
                interactions.areaPress = TextAreaPress{
                    .areaWidget = area.widgetId,
                    .homeOrigin = DragOrigin::Track};

                return;
            }

            if (contains(pageView.rect, *pointer.positionPoint))
            {
                interactions.areaPress = TextAreaPress{
                    .areaWidget = area.widgetId,
                    .homeOrigin = DragOrigin::Text};
            }
        }

        struct PaneReach final
        {
            std::uint64_t maxOffset = 0;

            std::uint64_t room = 0;

            std::uint64_t extent = 0;
        };

        [[nodiscard]] PaneReach reachOf(
            const LayoutTree &tree, const ScrollPane &pane) noexcept
        {
            const auto &viewport = tree.getNode(pane.viewport);
            const auto room =
                std::uint64_t{viewport.arrangedRect.size.height};

            std::optional<std::int64_t> top;
            std::int64_t bottom = 0;

            for (auto child = viewport.firstChild; child != kNoNode;
                 child = tree.getNode(child).nextSibling)
            {
                const auto &value = tree.getNode(child);

                if (value.overlayAnchor != kNoNode)
                {
                    continue;
                }

                if (!top)
                {
                    top = value.arrangedRect.originPoint.y;
                }

                bottom = value.arrangedRect.originPoint.y
                         + value.arrangedRect.size.height;
            }

            const auto extent =
                top.has_value() && bottom > *top
                                ? static_cast<std::uint64_t>(bottom - *top)
                                : 0;

            return PaneReach{
                .maxOffset = extent > room ? extent - room : 0,
                .room = room,
                .extent = extent};
        }

        [[nodiscard]] std::uint64_t getOffsetOnTrack(
            const Rect &trackRect,
            const Point point,
            const std::uint64_t maxOffset) noexcept
        {
            if (point.y <= trackRect.originPoint.y)
            {
                return 0;
            }

            const auto downOffset = static_cast<std::uint64_t>(
                point.y - trackRect.originPoint.y);
            const auto span = std::max(trackRect.size.height, 1U) - 1U;

            return getAcrossOffset(downOffset, span, maxOffset);
        }

        void placePaneThumb(
            LayoutTree &tree,
            const ScrollPane &pane,
            const PaneReach &reach,
            const std::uint64_t offset)
        {
            const auto track = tree.getNode(pane.track).arrangedRect;

            const auto trackHeight = track.size.height;

            const auto share =
                getAcrossOffset(reach.room, reach.extent, trackHeight);

            const auto thumb = std::clamp(
                share, std::min(trackHeight, pane.step), trackHeight);

            const auto slack = trackHeight - thumb;

            tree.getNode(pane.thumb).arrangedRect = Rect{
                .originPoint =
                    {.x = track.originPoint.x,
                     .y = track.originPoint.y
                          + static_cast<std::int32_t>(getAcrossOffset(
                              offset, reach.maxOffset, slack))},
                .size = {.width = track.size.width, .height = thumb}};
        }

        [[nodiscard]] std::uint32_t getValueOnRail(
            const Rect &trackRect,
            const Point point,
            const std::uint32_t range) noexcept
        {
            const auto span = std::max(trackRect.size.width, 1U) - 1U;

            if (point.x <= trackRect.originPoint.x)
            {
                return 0;
            }

            const auto alongOffset = static_cast<std::uint64_t>(
                point.x - trackRect.originPoint.x);

            return getAcrossOffset(std::min<std::uint64_t>(alongOffset, span), span,
                          range);
        }

        void placeRailThumb(
            LayoutTree &tree,
            const ScrollBar &railBar,
            const std::uint32_t value,
            const std::uint32_t width)
        {
            const auto track = tree.getNode(railBar.track).arrangedRect;

            const auto thumb = std::min(width, track.size.width);
            const auto slack = track.size.width - thumb;

            tree.getNode(railBar.thumb).arrangedRect = Rect{
                .originPoint =
                    {.x = track.originPoint.x
                          + static_cast<std::int32_t>(
                              getAcrossOffset(value, railBar.range, slack)),
                     .y = track.originPoint.y},
                .size = {.width = thumb, .height = track.size.height}};
        }

        [[nodiscard]] std::uint32_t getRatioOnBar(
            const Rect &boxRect,
            const Rect &dividerRect,
            const Point point,
            const Axis axis) noexcept
        {
            const auto room = axis == Axis::Row ? boxRect.size.width
                            : boxRect.size.height;
            const auto bar = axis == Axis::Row ? dividerRect.size.width
                           : dividerRect.size.height;

            if (room <= bar)
            {
                return 0;
            }

            const auto content = room - bar;
            const auto start = axis == Axis::Row ? boxRect.originPoint.x
                             : boxRect.originPoint.y;
            const auto herePixel = axis == Axis::Row ? point.x : point.y;

            const auto wantedOffset =
                static_cast<std::int64_t>(herePixel) - start
                                - static_cast<std::int64_t>(bar) / 2;

            const auto alongOffset = static_cast<std::uint64_t>(
                std::clamp<std::int64_t>(wantedOffset, 0, content));

            return static_cast<std::uint32_t>(
                alongOffset * kSplitRatioScale / content);
        }

        [[nodiscard]] std::size_t nodeFor(
            const LayoutTree &tree, const WidgetId widget) noexcept
        {
            if (widget == kNoWidget)
            {
                return kNoNode;
            }

            for (std::size_t index = 0; index < tree.getSize(); ++index)
            {
                if (tree.getNode(index).widgetId == widget)
                {
                    return index;
                }
            }

            return kNoNode;
        }

        [[nodiscard]] std::uint32_t getExtentOnEdge(
            const Rect &barRect,
            const Rect &panelRect,
            const Rect &roomRect,
            const Point point,
            const PanelEdge &panelEdge) noexcept
        {
            const auto axis = panelEdge.axis;
            const auto onRow = axis == Axis::Row;

            const auto thickness =
                onRow ? barRect.size.width : barRect.size.height;
            const auto barLeading =
                onRow ? barRect.originPoint.x : barRect.originPoint.y;
            const auto panelLeading =
                onRow ? panelRect.originPoint.x : panelRect.originPoint.y;
            const auto panelSide =
                onRow ? panelRect.size.width : panelRect.size.height;
            const auto herePixel = onRow ? point.x : point.y;

            const auto panelTrailing =
                panelLeading + static_cast<std::int32_t>(panelSide);
            const auto halfBar = static_cast<std::int64_t>(thickness) / 2;

            const auto wantedExtent =
                panelTrailing <= barLeading
                    ? static_cast<std::int64_t>(herePixel) - panelLeading
                          - halfBar
                    : static_cast<std::int64_t>(panelTrailing) - herePixel
                          - halfBar;

            const auto ceiling = panelEdge.maximum > 0
                               ? panelEdge.maximum
                               : (onRow ? roomRect.size.width
                                        : roomRect.size.height);

            return static_cast<std::uint32_t>(std::clamp<std::int64_t>(
                wantedExtent,
                panelEdge.minimum,
                std::max<std::int64_t>(ceiling, panelEdge.minimum)));
        }
    }

    void resolveAreas(
        LayoutTree &tree,
        const Pointer &pointer,
        const bool underOverlay,
        Interactions &interactions,
        std::optional<TextEdit> &edit)
    {
        for (const auto &area : tree.getAreas())
        {
            const auto pageView = showingOf(tree, area);

            const bool bar = area.track != kNoNode;

            const bool onTrack =
                bar
                && isHeldOnRect(
                    pointer,
                    underOverlay,
                    area.dragging == DragOrigin::Track,
                    tree.getNode(area.track).arrangedRect,
                    false);

            const auto top = getScrollLine(
                tree, area, pointer, pageView, onTrack, edit);

            if (top != area.requestedExtent && area.widgetId != kNoWidget)
            {
                interactions.scrollChange =
                    ScrollChange{.areaWidget = area.widgetId, .line = top};
            }

            layCaretAtPointer(area, pageView, pointer, underOverlay, edit);

            noteAreaPress(
                area, pageView, pointer, underOverlay, onTrack, interactions);

            if (bar)
            {
                placeThumb(tree, area, pageView);
            }
        }
    }

    void resolveRails(
        LayoutTree &tree,
        const Pointer &pointer,
        const bool underOverlay,
        const std::uint32_t thumbWidth,
        Interactions &interactions)
    {
        for (const auto &rail : tree.getRails())
        {
            auto value = rail.value;

            const bool onTrack = isHeldOnRect(
                pointer,
                underOverlay,
                rail.dragging,
                tree.getNode(rail.track).arrangedRect,
                false);

            if (onTrack && rail.range > 0)
            {
                value = getValueOnRail(
                    tree.getNode(rail.track).arrangedRect,
                    *pointer.positionPoint,
                    rail.range);
            }

            if (onTrack && rail.widgetId != kNoWidget)
            {
                interactions.slidChange =
                    SliderChange{.sliderWidget = rail.widgetId, .value = value};
            }

            placeRailThumb(tree, rail, value, thumbWidth);
        }
    }

    void resolvePanes(
        LayoutTree &tree,
        const Pointer &pointer,
        const bool underOverlay,
        Interactions &interactions)
    {
        for (const auto &pane : tree.getPanes())
        {
            const auto reach = reachOf(tree, pane);
            const auto viewportRect =
                tree.getNode(pane.viewport).arrangedRect;
            const auto trackRect = tree.getNode(pane.track).arrangedRect;

            auto offset = std::min<std::uint64_t>(
                tree.getNode(pane.viewport).scrollOffset.value_or(0),
                reach.maxOffset);

            const bool onTrack =
                reach.maxOffset > 0
                && isHeldOnRect(
                    pointer, underOverlay, pane.dragging, trackRect, false);

            if (onTrack)
            {
                offset = getOffsetOnTrack(
                    trackRect, *pointer.positionPoint, reach.maxOffset);
            }

            const bool wheeled =
                !underOverlay && pointer.scrolledSteps != 0
                && pointer.positionPoint
                && contains(viewportRect, *pointer.positionPoint);

            if (wheeled)
            {
                const auto movedOffset = std::max<std::int64_t>(
                    static_cast<std::int64_t>(offset)
                        + std::int64_t{pointer.scrolledSteps} * pane.step,
                    0);

                offset = std::min(
                    static_cast<std::uint64_t>(movedOffset),
                    reach.maxOffset);
            }

            if (pane.widgetId != kNoWidget
                && offset != pane.requestedOffset)
            {
                interactions.scrollChange = ScrollChange{
                    .areaWidget = pane.widgetId,
                    .line = static_cast<std::size_t>(offset)};
            }

            if (reach.maxOffset > 0 && !underOverlay && pointer.pressed
                && pointer.positionPoint
                && contains(trackRect, *pointer.positionPoint)
                && pane.widgetId != kNoWidget)
            {
                interactions.areaPress = TextAreaPress{
                    .areaWidget = pane.widgetId,
                    .homeOrigin = DragOrigin::Track};
            }

            if (reach.maxOffset == 0)
            {
                tree.getNode(pane.track).backgroundColor = {};
                tree.getNode(pane.thumb).backgroundColor = {};

                continue;
            }

            placePaneThumb(tree, pane, reach, offset);
        }
    }

    void resolveBars(
        const LayoutTree &tree,
        const Pointer &pointer,
        const bool underOverlay,
        Interactions &interactions)
    {
        for (const auto &bar : tree.getBars())
        {
            const bool taken = isHeldOnRect(
                pointer,
                underOverlay,
                bar.dragging,
                tree.getNode(bar.divider).arrangedRect,
                true);

            if (!taken || bar.widgetId == kNoWidget)
            {
                continue;
            }

            interactions.split = SplitChange{
                .dividerWidget = bar.widgetId,
                .ratio = getRatioOnBar(
                    tree.getNode(bar.split).arrangedRect,
                    tree.getNode(bar.divider).arrangedRect,
                    *pointer.positionPoint,
                    bar.axis)};
        }
    }

    void resolveEdges(
        const LayoutTree &tree,
        const Pointer &pointer,
        const bool underOverlay,
        Interactions &interactions)
    {
        for (const auto &panelEdge : tree.getEdges())
        {
            if (panelEdge.widgetId == kNoWidget)
            {
                continue;
            }

            const auto &barNode = tree.getNode(panelEdge.bar);

            const bool taken = isHeldOnRect(
                pointer,
                underOverlay,
                panelEdge.dragging,
                barNode.arrangedRect,
                true);

            if (!taken)
            {
                continue;
            }

            const auto panel = nodeFor(tree, panelEdge.panelWidget);

            if (panel == kNoNode)
            {
                continue;
            }

            interactions.edge = EdgeChange{
                .edgeWidget = panelEdge.widgetId,
                .extent = getExtentOnEdge(
                    barNode.arrangedRect,
                    tree.getNode(panel).arrangedRect,
                    tree.getNode(barNode.parent).arrangedRect,
                    *pointer.positionPoint,
                    panelEdge)};
        }
    }

    void applyVisualState(
        LayoutTree &tree,
        const Interactions &interactions,
        bool down)
    {
        for (std::size_t index = 0; index < tree.getSize(); ++index)
        {
            auto &node = tree.getNode(index);

            if (node.styleColors)
            {
                const bool under =
                    node.widgetId != kNoWidget
                    && node.widgetId == interactions.hoveredWidget;

                node.backgroundColor = fillFor(*node.styleColors, under, down);

                node.pressed = under && down;
            }

            const bool focused =
                interactions.focusedWidget != kNoWidget
                && node.widgetId == interactions.focusedWidget;

            node.focusRing = focused ? node.focusStyle
                           : std::optional<FocusRing>{};
        }
    }

}
