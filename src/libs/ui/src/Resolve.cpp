#include "Resolve.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/gfx/Rect.hpp>

#include "antwika/ui/Axis.hpp"
#include "antwika/ui/Keyboard.hpp"
#include "antwika/ui/OptionChoice.hpp"
#include "antwika/ui/ScrollChange.hpp"
#include "antwika/ui/SliderChange.hpp"
#include "antwika/ui/SplitChange.hpp"
#include "antwika/ui/SplitSpec.hpp"
#include "antwika/ui/TextEdit.hpp"
#include "antwika/ui/WidgetId.hpp"

#include "Area.hpp"
#include "Bar.hpp"
#include "Contains.hpp"
#include "FocusRing.hpp"
#include "Interactive.hpp"
#include "Rail.hpp"
#include "Saturate.hpp"
#include "TextEditing.hpp"

namespace antwika::ui::detail
{

    namespace
    {
        [[nodiscard]] Color fillFor(
            const Interactive &style, bool under, bool down) noexcept
        {
            if (!under)
            {
                return style.idle;
            }

            return down ? style.pressed : style.hovered;
        }

        [[nodiscard]] std::vector<WidgetId> focusableIds(
            const LayoutTree &tree)
        {
            std::vector<WidgetId> ids;

            for (std::size_t index = 0; index < tree.size(); ++index)
            {
                const auto &node = tree.node(index);

                if (!node.focusStyle || node.id == kNoWidget)
                {
                    continue;
                }

                if (std::ranges::find(ids, node.id) == ids.end())
                {
                    ids.push_back(node.id);
                }
            }

            return ids;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] bool listed(
            const std::vector<WidgetId> &ids, WidgetId id)
        {
            return std::ranges::find(ids, id) != ids.end();
        }

        [[nodiscard]] WidgetId step(
            const std::vector<WidgetId> &ids, WidgetId from, bool forward)
        {
            if (ids.empty())
            {
                return kNoWidget;
            }

            if (from == kNoWidget)
            {
                return forward ? ids.front() : ids.back();
            }

            const auto at = std::ranges::find(ids, from);
            const auto index = static_cast<std::size_t>(at - ids.begin());
            const auto next = forward ? index + 1 : index + ids.size() - 1;

            return ids[next % ids.size()];
        }

        [[nodiscard]] bool hitTest(
            const LayoutTree &tree,
            const Pointer &pointer,
            Interactions &interactions)
        {
            std::optional<OptionChoice> option;

            const auto scan = [&](bool overlay) {
                bool contained = false;

                for (std::size_t index = tree.size(); index-- > 0;)
                {
                    const auto &node = tree.node(index);

                    if (node.overlay != overlay
                        || !contains(node.arranged, *pointer.position))
                    {
                        continue;
                    }

                    contained = true;

                    if (node.background)
                    {
                        interactions.pointerOverUi = true;
                    }

                    if (node.id != kNoWidget
                        && interactions.hovered == kNoWidget)
                    {
                        interactions.hovered = node.id;
                    }

                    if (node.optionOwner != kNoWidget && !option)
                    {
                        option = OptionChoice{
                            .dropdown = node.optionOwner,
                            .index = node.optionIndex};
                    }
                }

                return contained;
            };

            bool underOverlay = false;

            if (pointer.position)
            {
                underOverlay = scan(true);

                if (!underOverlay)
                {
                    scan(false);
                }
            }

            if (pointer.pressed)
            {
                interactions.activated = interactions.hovered;
                interactions.chosen = option;
            }

            return underOverlay;
        }

        [[nodiscard]] std::optional<OptionChoice> optionFor(
            const LayoutTree &tree, WidgetId id)
        {
            for (std::size_t index = 0; index < tree.size(); ++index)
            {
                const auto &node = tree.node(index);

                if (node.id == id && node.optionOwner != kNoWidget)
                {
                    return OptionChoice{
                        .dropdown = node.optionOwner,
                        .index = node.optionIndex};
                }
            }

            return std::nullopt;
        }

        void resolveFocus(
            const LayoutTree &tree,
            const Keyboard &keyboard,
            WidgetId focus,
            Interactions &interactions)
        {
            const bool focusInPlay =
                focus != kNoWidget || !keyboard.keys.empty();

            if (focusInPlay && interactions.activated != kNoWidget)
            {
                focus = interactions.activated;
            }

            const auto focusables = focusableIds(tree);

            if (!listed(focusables, focus))
            {
                focus = kNoWidget;
            }

            for (const auto key : keyboard.keys)
            {
                if (key == Key::Activate)
                {
                    if (focus != kNoWidget)
                    {
                        interactions.activated = focus;

                        if (auto option = optionFor(tree, focus))
                        {
                            interactions.chosen = option;
                        }
                    }

                    continue;
                }

                if (key == Key::FocusNext || key == Key::FocusPrevious)
                {
                    focus =
                        step(focusables, focus, key == Key::FocusNext);
                }
            }

            interactions.focused = focus;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] std::size_t indexAt(
            const std::string_view text,
            const std::size_t line,
            const std::size_t column) noexcept
        {
            std::size_t begin = 0;

            for (std::size_t at = 0; at < line; ++at)
            {
                const auto end = endOfLine(text, begin);

                if (end == text.size())
                {
                    return text.size();
                }

                begin = end + 1;
            }

            return std::min(begin + column, endOfLine(text, begin));
        }

        [[nodiscard]] std::size_t lineOf(
            const std::string_view text, const std::size_t at) noexcept
        {
            return static_cast<std::size_t>(
                std::ranges::count(text.substr(0, at), '\n'));
        }

        [[nodiscard]] std::uint32_t across(
            std::uint64_t value,
            std::uint64_t range,
            std::uint64_t span) noexcept
        {
            if (range == 0)
            {
                return 0;
            }

            return clampToU32(std::min(value, range) * span / range);
        }

        [[nodiscard]] std::size_t rowsOf(
            const Area &area, const std::size_t line) noexcept
        {
            std::size_t rows = 1;

            for (const auto &band : area.bands)
            {
                if (band.line == line)
                {
                    rows += band.rows;
                }
            }

            return rows;
        }

        [[nodiscard]] std::size_t rowsBefore(
            const Area &area, const std::size_t line) noexcept
        {
            std::size_t rows = 0;

            for (std::size_t at = 0; at < line; ++at)
            {
                rows += rowsOf(area, at);
            }

            return rows;
        }

        struct Showing final
        {
            std::size_t page = 1;

            std::size_t furthest = 0;

            Rect column{};
        };

        [[nodiscard]] Showing showingOf(
            const LayoutTree &tree, const Area &area) noexcept
        {
            const auto &column = tree.node(area.column).arranged;

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

            return Showing{
                .page = page,
                .furthest = furthest,
                .column = column};
        }

        void placeThumb(
            LayoutTree &tree, const Area &area, const Showing &showing)
        {
            const auto track = tree.node(area.track).arranged;

            const auto whole = track.size.height;

            const auto share = across(
                showing.page, rowsBefore(area, area.lines), whole);

            const auto thumb =
                std::clamp(share, std::min(whole, area.lineHeight), whole);

            const auto slack = whole - thumb;

            tree.node(area.thumb).arranged = Rect{
                .origin =
                    {.x = track.origin.x,
                     .y = track.origin.y
                          + static_cast<std::int32_t>(across(
                              rowsBefore(area, area.scroll),
                              rowsBefore(area, showing.furthest),
                              slack))},
                .size = {.width = track.size.width, .height = thumb}};
        }

        [[nodiscard]] std::size_t lineOnTrack(
            const Rect &track,
            const Point at,
            const std::size_t furthest) noexcept
        {
            const auto down = static_cast<std::uint64_t>(
                at.y - track.origin.y);

            return across(
                down, std::max(track.size.height, 1U) - 1U, furthest);
        }

        void layCaret(
            const Area &area,
            const Showing &showing,
            const Point at,
            const bool extends,
            std::optional<TextEdit> &edit)
        {
            const auto down = static_cast<std::uint64_t>(
                static_cast<std::int64_t>(at.y)
                - showing.column.origin.y);

            const auto right = static_cast<std::uint64_t>(
                static_cast<std::int64_t>(at.x)
                - showing.column.origin.x);

            auto next = edit.has_value() && edit->field == area.id
                ? *edit
                : TextEdit{ // GCOVR_EXCL_LINE
                      .field = area.id,
                      .text = std::string{area.text}, // GCOVR_EXCL_LINE
                      .cursor = area.cursor,
                      .anchor = area.anchor};

            auto remaining =
                static_cast<std::size_t>(down / area.lineHeight);
            auto line = area.scroll;

            while (line < area.lines
                   && remaining >= rowsOf(area, line))
            {
                remaining -= rowsOf(area, line);
                ++line;
            }

            next.cursor = indexAt(
                next.text,
                line,
                static_cast<std::size_t>(right / area.advance));

            if (!extends)
            {
                next.anchor = next.cursor;
            }

            if (!edit.has_value() && next.cursor == area.cursor
                && next.anchor == area.anchor)
            {
                return;
            }

            edit = next;
        }

        void resolveAreas(
            LayoutTree &tree,
            const Pointer &pointer,
            const bool underOverlay,
            Interactions &interactions,
            std::optional<TextEdit> &edit)
        {
            for (const auto &area : tree.areas())
            {
                const auto showing = showingOf(tree, area);

                auto top = std::min(area.scroll, showing.furthest);

                const bool bar = area.track != kNoNode;

                const bool onTrack =
                    !underOverlay && bar && pointer.position
                    && pointer.down
                    && (pointer.pressed
                        || area.dragging == DragHome::Track)
                    && contains(
                        tree.node(area.track).arranged, *pointer.position);

                if (onTrack)
                {
                    top = lineOnTrack(
                        tree.node(area.track).arranged,
                        *pointer.position,
                        showing.furthest);
                }

                const bool moved = !onTrack && edit.has_value()
                                   && edit->field == area.id
                                   && area.id != kNoWidget;

                if (moved)
                {
                    const auto line = lineOf(edit->text, edit->cursor);

                    std::size_t least = line;
                    std::size_t rows = 1;

                    while (least > 0
                           && rows + rowsOf(area, least - 1)
                                  <= showing.page)
                    {
                        rows += rowsOf(area, least - 1);
                        --least;
                    }

                    top = std::clamp(top, least, line);
                }

                if (top != area.requested && area.id != kNoWidget)
                {
                    interactions.scrolled =
                        ScrollChange{.area = area.id, .line = top};
                }

                const bool inText =
                    !underOverlay && area.focused && pointer.position
                    && (pointer.pressed
                        || (pointer.down && pointer.extends
                            && area.dragging == DragHome::Text))
                    && contains(showing.column, *pointer.position);

                if (inText)
                {
                    layCaret(
                        area,
                        showing,
                        *pointer.position,
                        pointer.extends,
                        edit);
                }

                if (!underOverlay && pointer.pressed && pointer.position
                    && area.id != kNoWidget)
                {
                    if (onTrack)
                    {
                        interactions.areaPress = AreaPress{
                            .area = area.id, .home = DragHome::Track};
                    }
                    else if (contains(
                                 showing.column, *pointer.position))
                    {
                        interactions.areaPress = AreaPress{
                            .area = area.id, .home = DragHome::Text};
                    }
                }

                if (bar)
                {
                    placeThumb(tree, area, showing);
                }
            }
        }

        [[nodiscard]] std::uint32_t valueOnRail(
            const Rect &track,
            const Point at,
            const std::uint32_t range) noexcept
        {
            const auto span = std::max(track.size.width, 1U) - 1U;

            if (at.x <= track.origin.x)
            {
                return 0;
            }

            const auto along = static_cast<std::uint64_t>(
                at.x - track.origin.x);

            return across(std::min<std::uint64_t>(along, span), span,
                          range);
        }

        void placeRailThumb(
            LayoutTree &tree,
            const Rail &rail,
            const std::uint32_t value,
            const std::uint32_t width)
        {
            const auto track = tree.node(rail.track).arranged;

            const auto thumb = std::min(width, track.size.width);
            const auto slack = track.size.width - thumb;

            tree.node(rail.thumb).arranged = Rect{
                .origin =
                    {.x = track.origin.x
                          + static_cast<std::int32_t>(
                              across(value, rail.range, slack)),
                     .y = track.origin.y},
                .size = {.width = thumb, .height = track.size.height}};
        }

        void resolveRails(
            LayoutTree &tree,
            const Pointer &pointer,
            const bool underOverlay,
            const std::uint32_t thumbWidth,
            Interactions &interactions)
        {
            for (const auto &rail : tree.rails())
            {
                auto value = rail.value;

                const bool onTrack =
                    !underOverlay && pointer.position && pointer.down
                    && (pointer.pressed || rail.dragging)
                    && contains(
                           tree.node(rail.track).arranged,
                           *pointer.position);

                if (onTrack && rail.range > 0)
                {
                    value = valueOnRail(
                        tree.node(rail.track).arranged,
                        *pointer.position,
                        rail.range);
                }

                if (onTrack && rail.id != kNoWidget)
                {
                    interactions.slid =
                        SliderChange{.slider = rail.id, .value = value};
                }

                placeRailThumb(tree, rail, value, thumbWidth);
            }
        }

        [[nodiscard]] std::uint32_t ratioOnBar(
            const Rect &box,
            const Rect &divider,
            const Point at,
            const Axis axis) noexcept
        {
            const auto room = axis == Axis::Row ? box.size.width
                                                : box.size.height;
            const auto bar = axis == Axis::Row ? divider.size.width
                                               : divider.size.height;

            if (room <= bar)
            {
                return 0;
            }

            const auto content = room - bar;
            const auto start = axis == Axis::Row ? box.origin.x
                                                 : box.origin.y;
            const auto here = axis == Axis::Row ? at.x : at.y;

            const auto wanted = static_cast<std::int64_t>(here) - start
                                - static_cast<std::int64_t>(bar) / 2;

            const auto along = static_cast<std::uint64_t>(
                std::clamp<std::int64_t>(wanted, 0, content));

            return static_cast<std::uint32_t>(
                along * kWholeSplit / content);
        }

        void resolveBars(
            const LayoutTree &tree,
            const Pointer &pointer,
            const bool underOverlay,
            Interactions &interactions)
        {
            for (const auto &bar : tree.bars())
            {
                if (underOverlay || !pointer.position || !pointer.down)
                {
                    continue;
                }

                const bool taken =
                    bar.dragging
                    || (pointer.pressed
                        && contains(
                               tree.node(bar.divider).arranged,
                               *pointer.position));

                if (!taken || bar.id == kNoWidget)
                {
                    continue;
                }

                interactions.split = SplitChange{
                    .divider = bar.id,
                    .ratio = ratioOnBar(
                        tree.node(bar.split).arranged,
                        tree.node(bar.divider).arranged,
                        *pointer.position,
                        bar.axis)};
            }
        }

        void dress(
            LayoutTree &tree,
            const Interactions &interactions,
            bool down)
        {
            for (std::size_t index = 0; index < tree.size(); ++index)
            {
                auto &node = tree.node(index);

                if (node.style)
                {
                    const bool under =
                        node.id != kNoWidget
                        && node.id == interactions.hovered;

                    node.background = fillFor(*node.style, under, down);

                    node.pressed = under && down;
                }

                const bool focused =
                    interactions.focused != kNoWidget
                    && node.id == interactions.focused;

                node.focusRing = focused ? node.focusStyle
                                         : std::optional<FocusRing>{};
            }
        }
    }

    Interactions resolve(
        LayoutTree &tree,
        const Pointer &pointer,
        const Keyboard &keyboard,
        WidgetId focus,
        std::optional<TextEdit> &edit,
        std::uint32_t thumbWidth)
    {
        Interactions interactions;

        const bool underOverlay = hitTest(tree, pointer, interactions);

        resolveFocus(tree, keyboard, focus, interactions);
        resolveAreas(tree, pointer, underOverlay, interactions, edit);
        resolveRails(
            tree, pointer, underOverlay, thumbWidth, interactions);
        resolveBars(tree, pointer, underOverlay, interactions);
        dress(tree, interactions, pointer.down);

        return interactions;
    } // GCOVR_EXCL_LINE

}
