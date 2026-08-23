#include "ResolveWidgets.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include <antwika/gfx/Rect.hpp>

#include "antwika/ui/Axis.hpp"
#include "antwika/ui/ScrollChange.hpp"
#include "antwika/ui/SliderChange.hpp"
#include "antwika/ui/SplitChange.hpp"
#include "antwika/ui/SplitSpec.hpp"
#include "antwika/ui/TextEdit.hpp"
#include "antwika/ui/WidgetId.hpp"

#include "Area.hpp"
#include "Splitter.hpp"
#include "Contains.hpp"
#include "FocusRing.hpp"
#include "StateColors.hpp"
#include "ScrollBar.hpp"
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

        [[nodiscard]] std::uint32_t getAcross(
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

            const auto share = getAcross(
                pageView.page, getRowsBefore(area, area.lines), trackHeight);

            const auto thumb =
                std::clamp(share, std::min(trackHeight, area.lineHeight),
                    trackHeight);

            const auto slack = trackHeight - thumb;

            tree.getNode(area.thumb).arrangedRect = Rect{
                .originPoint =
                    {.x = track.originPoint.x,
                     .y = track.originPoint.y
                          + static_cast<std::int32_t>(getAcross(
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

            return getAcross(
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
                      .text = std::string{area.text}, // GCOVR_EXCL_LINE
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

            return getAcross(std::min<std::uint64_t>(alongOffset, span), span,
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
                              getAcross(value, railBar.range, slack)),
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

            auto top = std::min(area.scroll, pageView.furthest);

            const bool bar = area.track != kNoNode;

            const bool onTrack =
                !underOverlay && bar && pointer.positionPoint
                && pointer.down
                && (pointer.pressed
                    || area.dragging == DragOrigin::Track)
                && contains(
                    tree.getNode(area.track).arrangedRect, *pointer.positionPoint);

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

            if (moved)
            {
                const auto line = lineOf(edit->text, edit->cursor);

                std::size_t least = line;
                std::size_t rows = 1;

                while (least > 0
                       && rows + rowsOf(area, least - 1)
                              <= pageView.page)
                {
                    rows += rowsOf(area, least - 1);
                    --least;
                }

                top = std::clamp(top, least, line);
            }

            if (top != area.requestedExtent && area.widgetId != kNoWidget)
            {
                interactions.scrollChange =
                    ScrollChange{.areaWidget = area.widgetId, .line = top};
            }

            const bool inText =
                !underOverlay && area.focused && pointer.positionPoint
                && (pointer.pressed
                    || (pointer.down && pointer.extendsSelection
                        && area.dragging == DragOrigin::Text))
                && contains(pageView.rect, *pointer.positionPoint);

            if (inText)
            {
                layCaret(
                    area,
                    pageView,
                    *pointer.positionPoint,
                    pointer.extendsSelection,
                    edit);
            }

            if (!underOverlay && pointer.pressed && pointer.positionPoint
                && area.widgetId != kNoWidget)
            {
                if (onTrack)
                {
                    interactions.areaPress = TextAreaPress{
                        .areaWidget = area.widgetId,
                        .homeOrigin = DragOrigin::Track};
                }
                else if (contains(
                             pageView.rect, *pointer.positionPoint))
                {
                    interactions.areaPress = TextAreaPress{
                        .areaWidget = area.widgetId,
                        .homeOrigin = DragOrigin::Text};
                }
            }

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

            const bool onTrack =
                !underOverlay && pointer.positionPoint && pointer.down
                && (pointer.pressed || rail.dragging)
                && contains(
                       tree.getNode(rail.track).arrangedRect,
                       *pointer.positionPoint);

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

    void resolveBars(
        const LayoutTree &tree,
        const Pointer &pointer,
        const bool underOverlay,
        Interactions &interactions)
    {
        for (const auto &bar : tree.getBars())
        {
            if (underOverlay || !pointer.positionPoint || !pointer.down)
            {
                continue;
            }

            const bool taken =
                bar.dragging
                || (pointer.pressed
                    && contains(
                           tree.getNode(bar.divider).arrangedRect,
                           *pointer.positionPoint));

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
