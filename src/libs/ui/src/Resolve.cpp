#include "Resolve.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/gfx/Rect.hpp>

#include "antwika/ui/Keyboard.hpp"
#include "antwika/ui/OptionChoice.hpp"
#include "antwika/ui/ScrollChange.hpp"
#include "antwika/ui/TextEdit.hpp"
#include "antwika/ui/WidgetId.hpp"

#include "Area.hpp"
#include "Contains.hpp"
#include "FocusRing.hpp"
#include "Interactive.hpp"
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

        /**
         * @brief List what the keyboard can reach, in declaration order.
         *
         * Ascending index is the order the caller wrote the widgets in,
         * so this is the tab order and no second one has to be kept in
         * step with the layout.
         *
         * A repeated id is listed once: two nodes sharing an id are one
         * widget, so Tab must not stop at it twice.
         *
         * @param tree The arena, already laid out.
         * @return The focusable ids, in declaration order.
         */
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
            // Only an unwind destroys ids at this brace.
        } // GCOVR_EXCL_LINE

        /**
         * @brief Check whether an id is in the tab order.
         * @param ids The tab order.
         * @param id The id to look for.
         * @return True when the id is one the keyboard can reach.
         */
        [[nodiscard]] bool listed(
            const std::vector<WidgetId> &ids, WidgetId id)
        {
            return std::ranges::find(ids, id) != ids.end();
        }

        /**
         * @brief Move focus one widget along, wrapping at either end.
         *
         * @param ids The tab order.
         * @param from The widget focused now, which is either kNoWidget
         * or one of ids: resolve() drops anything else first.
         * @param forward True for Tab, false for Shift+Tab.
         * @return The widget to focus next.
         */
        [[nodiscard]] WidgetId step(
            const std::vector<WidgetId> &ids, WidgetId from, bool forward)
        {
            if (ids.empty())
            {
                return kNoWidget;
            }

            // Nothing focused yet, so both arrive from outside.
            // Tab takes the first widget and Shift+Tab the last.
            if (from == kNoWidget)
            {
                return forward ? ids.front() : ids.back();
            }

            const auto at = std::ranges::find(ids, from);
            const auto index = static_cast<std::size_t>(at - ids.begin());
            const auto next = forward ? index + 1 : index + ids.size() - 1;

            return ids[next % ids.size()];
        }

        /**
         * @brief Stage one: what the pointer is on and what it pressed.
         *
         * Reads the arranged arena and the pointer, and writes the
         * hovered widget, the covered flag, and -- on a press -- the
         * activated widget and the chosen option.
         * Touches no node, so nothing it decides depends on anything a
         * later stage writes.
         *
         * @param tree The arranged arena.
         * @param pointer What the caller reports about the pointer.
         * @param interactions Receives this stage's answers.
         * @return Whether an overlay claimed the pointer; the stage
         * reading clicks against areas suppresses itself on it.
         */
        [[nodiscard]] bool hitTest(
            const LayoutTree &tree,
            const Pointer &pointer,
            Interactions &interactions)
        {
            std::optional<OptionChoice> option;

            // An overlay is painted after everything else.
            // So it is in front, and so it is hit first.
            // Two passes are what say that.
            // One descending loop can only mean the arena's own order.
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

                    // An option reports its index rather than its id.
                    // So it is tracked apart from the hovered widget.
                    // An unnamed option therefore still answers.
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
                // A pointer an overlay claims never reaches beneath it.
                // The overlay is in front of the base layer, named or not.
                // So the base layer stays unread.
                underOverlay = scan(true);

                if (!underOverlay)
                {
                    scan(false);
                }
            }

            // Nothing hovered means there is nothing to activate.
            // So a press over no widget copies kNoWidget, no guard.
            if (pointer.pressed)
            {
                interactions.activated = interactions.hovered;
                interactions.chosen = option;
            }

            return underOverlay;
        }

        /**
         * @brief Find the option a focusable widget is, if it is one.
         *
         * An option carries its owner and its index as well as its id,
         * so this is a lookup rather than arithmetic: a caller must
         * never have to subtract DropdownSpec::optionIdBase from an id
         * to work out which option was meant.
         *
         * @param tree The arena, already laid out.
         * @param id The focused widget's id, never kNoWidget.
         * @return What a press on that widget would have chosen, or
         * nothing when it is not an option.
         */
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

        /**
         * @brief Stage two: where focus ends up, and what Enter did.
         *
         * Runs after the hit-test because a press may take focus with
         * it, and before the dressing because the ring goes on whatever
         * this decides. Enter is reported through Interactions::
         * activated, which the hit-test may already have written, so
         * this stage adds to that answer rather than replacing it.
         *
         * @param tree The arena, for the tab order it declares and for
         * what a focused option says it is.
         * @param keyboard The key edges, in arrival order.
         * @param focus The widget focused going in.
         * @param interactions Receives the focused widget, and -- when
         * Enter arrived -- the activated one and, if that widget is a
         * dropdown option, the chosen one.
         */
        void resolveFocus(
            const LayoutTree &tree,
            const Keyboard &keyboard,
            WidgetId focus,
            Interactions &interactions)
        {
            // Focus is in play once the caller has some or sends a key.
            // Only then does the pointer take focus along with it.
            // So tabbing on carries on from the button that was clicked.
            // A caller using the pointer alone is left exactly as it was.
            const bool focusInPlay =
                focus != kNoWidget || !keyboard.keys.empty();

            if (focusInPlay && interactions.activated != kNoWidget)
            {
                focus = interactions.activated;
            }

            const auto focusables = focusableIds(tree);

            // A focus on a widget this frame did not declare is dropped.
            // The layout is described afresh, so a widget gone is gone.
            // kNoWidget is never listed, so nothing-focused stays put.
            if (!listed(focusables, focus))
            {
                focus = kNoWidget;
            }

            for (const auto key : keyboard.keys)
            {
                if (key == Key::Activate)
                {
                    // Reported through the field a press uses.
                    // So a click and a key land in one place.
                    if (focus != kNoWidget)
                    {
                        interactions.activated = focus;

                        // And through the second field a press uses.
                        // An id alone cannot carry an option's index.
                        // Reporting one leaves the caller reversing it.
                        if (auto option = optionFor(tree, focus))
                        {
                            interactions.chosen = option;
                        }
                    }

                    continue;
                }

                // The editing keys belong to whatever is focused.
                // They are read where a text field is declared.
                // So focus only ever moves on the two that name it.
                if (key == Key::FocusNext || key == Key::FocusPrevious)
                {
                    focus =
                        step(focusables, focus, key == Key::FocusNext);
                }
            }

            interactions.focused = focus;
            // Only an unwind destroys focusables at this brace.
            // Nothing after its construction throws.
        } // GCOVR_EXCL_LINE

        /**
         * @brief Get the index a click at one line and column lands on.
         *
         * Total in both arguments: a line past the last one is the end
         * of the text, and a column past a line's end is that line's
         * end. Clicking in the empty part of an area therefore lands
         * somewhere real rather than nowhere.
         *
         * @param text The whole document.
         * @param line Which line was clicked, counting from zero.
         * @param column Which glyph cell across it was.
         * @return The index into text.
         */
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

        /**
         * @brief Get which line of a text an index is on.
         * @param text The document to count in.
         * @param at An index into it.
         * @return The line, counting from zero.
         */
        [[nodiscard]] std::size_t lineOf(
            const std::string_view text, const std::size_t at) noexcept
        {
            return static_cast<std::size_t>(
                std::ranges::count(text.substr(0, at), '\n'));
        }

        /**
         * @brief How far one number goes between zero and another.
         *
         * Done in 64 bits, since both are pixel counts and the product
         * of two of them is not one.
         *
         * @param value How far along.
         * @param range What that is a fraction of; zero answers zero.
         * @param span What to scale it up to.
         * @return The scaled value, never above span.
         */
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

        /**
         * @brief Get how many rows one line of an area stands on.
         *
         * Its own row, and every row its bands hold open under it.
         * Everything mapping rows to lines counts through this, so a
         * band moves what is under it in the layout and in the
         * arithmetic alike.
         *
         * @param area The area whose bands to count.
         * @param line The line, counting from zero.
         * @return The rows, at least one.
         */
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

        /**
         * @brief Get how many rows the lines above one line stand on.
         * @param area The area whose bands to count.
         * @param line The first line not counted.
         * @return The rows, bands included.
         */
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

        /**
         * @brief What one area's own state comes to once it is laid
         * out.
         *
         * Everything below reads this rather than working the same
         * numbers out again: how many rows are showing is what decides
         * how far the area can scroll, where its thumb goes, and which
         * line a click is on.
         */
        struct Showing
        {
            /** @brief How many whole rows fit, never zero. */
            std::size_t page = 1;

            /** @brief The furthest line that can usefully be at top. */
            std::size_t furthest = 0;

            /** @brief Where the lines were drawn. */
            Rect column{};
        };

        [[nodiscard]] Showing showingOf(
            const LayoutTree &tree, const Area &area) noexcept
        {
            const auto &column = tree.node(area.column).arranged;

            // An area too short for a whole row still shows one.
            // Nothing below could divide by it otherwise.
            const auto page = std::max<std::size_t>(
                column.size.height / area.lineHeight, 1);

            // Walked in rows from the end rather than subtracted.
            // A line with a band under it stands on more rows than one.
            // So the last page holds however many lines fit.
            std::size_t furthest = area.lines;
            std::size_t rows = 0;

            while (furthest > 0
                   && rows + rowsOf(area, furthest - 1) <= page)
            {
                rows += rowsOf(area, furthest - 1);
                --furthest;
            }

            // A last line whose band outgrows the page walks nowhere.
            // The top line it can be is still a real line.
            furthest = std::min(furthest, area.lines - 1);

            return Showing{
                .page = page,
                .furthest = furthest,
                .column = column};
        }

        /**
         * @brief Put the scrollbar's thumb where the drawn lines say.
         *
         * As long a share of the track as is showing of the document,
         * and as far down it as the top line is through the ones that
         * could be at the top. Written straight onto the arranged node
         * because both of those need the track's own height, which the
         * layout has only just decided.
         *
         * @param tree The arranged arena; the thumb's rectangle is
         * written.
         * @param area The area, which must have a bar.
         * @param showing What it came out showing.
         */
        void placeThumb(
            LayoutTree &tree, const Area &area, const Showing &showing)
        {
            const auto track = tree.node(area.track).arranged;

            const auto whole = track.size.height;

            // In rows rather than lines, so a band counts as content.
            const auto share = across(
                showing.page, rowsBefore(area, area.lines), whole);

            // Never thinner than a line.
            // A long document would leave nothing to take hold of.
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

        /**
         * @brief Get the line a press on the scrollbar asks for.
         * @param track Where the bar was drawn.
         * @param at Where the pointer is, inside it.
         * @param furthest The furthest line that can be at the top.
         * @return The line to put at the top.
         */
        [[nodiscard]] std::size_t lineOnTrack(
            const Rect &track,
            const Point at,
            const std::size_t furthest) noexcept
        {
            const auto down = static_cast<std::uint64_t>(
                at.y - track.origin.y);

            // One less than the height.
            // So the track's last pixel is the last line.
            return across(
                down, std::max(track.size.height, 1U) - 1U, furthest);
        }

        /**
         * @brief Put a focused area's caret where the pointer is.
         *
         * Amends whatever the keys already came to this frame rather
         * than replacing it, so a frame that both typed and clicked
         * reports one edit holding both -- and an area nothing was
         * typed into reports the move on its own.
         *
         * @param area The area, whose text the click is measured
         * against.
         * @param showing Where its lines were drawn.
         * @param at Where the pointer is, inside them.
         * @param extends Whether to leave the selection's far end where
         * it is rather than bringing it along.
         * @param edit The frame's edit, which is written.
         */
        void layCaret(
            const Area &area,
            const Showing &showing,
            const Point at,
            const bool extends,
            std::optional<TextEdit> &edit)
        {
            // contains() has already said both of these are inside.
            // So both differences are below an extent.
            // Sixty-four bits because an int32 minus an int32 is not one.
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

            // Which line the clicked row was drawn on, bands counted.
            // A row inside a line's band belongs to that line.
            // One past every line walks out, onto the text's end.
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

            // A click where the caret already is changed nothing.
            // And a frame that changed nothing reports nothing.
            // Which is the rule every keystroke here follows.
            if (!edit.has_value() && next.cursor == area.cursor
                && next.anchor == area.anchor)
            {
                return;
            }

            edit = next;
        }

        /**
         * @brief Stage three: what a click did inside a text area, and
         * which line each one is showing at its top.
         *
         * The one stage that needs the layout for something other than
         * a hit-test: which character a click landed on is a function
         * of where the area ended up, and so is how much of a document
         * is showing. Both are settled here rather than where the area
         * was declared, and both come back the way everything else
         * does -- through the reported edit, and through
         * Interactions::scrolled.
         *
         * @param tree The arranged arena; a thumb's rectangle is
         * written.
         * @param pointer What the caller reports about the pointer.
         * @param underOverlay Whether an overlay claimed the pointer;
         * a claimed pointer reaches no track and no text, since what
         * it is over is the overlay and not the area beneath.
         * @param interactions Receives the scroll report.
         * @param edit The edit the areas and fields reported where they
         * were declared, which a click on an area's text amends.
         */
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

                // A held drag is scoped to where its press landed.
                // The spec's dragging is the caller handing that back.
                // A fresh press needs no scoping; it lands where it is.
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

                // A caret that has just moved is brought into view.
                // One that has not is left exactly where it is.
                // So a bar drag may take the text off the caret.
                // Nothing pulls it straight back.
                const bool moved = !onTrack && edit.has_value()
                                   && edit->field == area.id
                                   && area.id != kNoWidget;

                if (moved)
                {
                    const auto line = lineOf(edit->text, edit->cursor);

                    // The nearest top that still shows the line.
                    // Walked in rows, so the bands above it count.
                    // The line's own band may hang past the page.
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

                // Never for an unnamed area.
                // A report naming kNoWidget says nothing usable.
                // And it would shadow a named area's real answer.
                if (top != area.requested && area.id != kNoWidget)
                {
                    interactions.scrolled =
                        ScrollChange{.area = area.id, .line = top};
                }

                // On the lines as they were drawn.
                // The click landed on what was on the screen.
                // Unless an overlay was over it: then it landed there.
                // A held drag reaches the text only if it began there.
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

                // Which part of this area the press landed on.
                // Telling the caller scopes the drag that follows.
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

        /**
         * @brief Stage three: write every node's resolved appearance.
         *
         * The only stage that writes to the arena, and it reads nothing
         * out of it that the first two did not already decide. Both
         * appearances are written on every node rather than only on the
         * ones that changed, so resolving the same arena twice leaves it
         * saying the same thing.
         *
         * @param tree The arranged arena; backgrounds and rings are
         * written.
         * @param interactions What the first two stages decided.
         * @param down Whether a pointer button is being held.
         */
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
                    // An unnamed widget can never be the hovered one.
                    // Comparing two kNoWidgets would say otherwise.
                    const bool under =
                        node.id != kNoWidget
                        && node.id == interactions.hovered;

                    node.background = fillFor(*node.style, under, down);

                    // Written beside the colour it explains.
                    // A hover pass reads it to step over a held widget.
                    node.pressed = under && down;
                }

                // The focused id is a listed one or kNoWidget.
                // No listed id is kNoWidget, so unnamed nodes stay bare.
                const bool focused =
                    interactions.focused != kNoWidget
                    && node.id == interactions.focused;

                node.focusRing = focused ? node.focusStyle
                                         : std::optional<FocusRing>{};
            }
        }
    } // namespace

    Interactions resolve(
        LayoutTree &tree,
        const Pointer &pointer,
        const Keyboard &keyboard,
        WidgetId focus,
        std::optional<TextEdit> &edit)
    {
        // Four stages, and only this order works.
        // A press may move focus, so the pointer is read first.
        // The ring goes on whatever focus ended up being.
        // So focus is settled before anything is dressed.
        // The areas read the edit the keys came to and amend it.
        // So they run before the caller reads it.
        // Dressing is the only one that writes an appearance.
        Interactions interactions;

        const bool underOverlay = hitTest(tree, pointer, interactions);

        resolveFocus(tree, keyboard, focus, interactions);
        resolveAreas(tree, pointer, underOverlay, interactions, edit);
        dress(tree, interactions, pointer.down);

        return interactions;
        // Interactions carries an optional edit, and so a string.
        // Only an unwind destroys one at this brace.
        // Nothing between its construction and the return throws.
    } // GCOVR_EXCL_LINE

} // namespace antwika::ui::detail
