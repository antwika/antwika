#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/gfx/Glyphs.hpp>

#include "antwika/ui/Axis.hpp"
#include "antwika/ui/Context.hpp"
#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/TextAreaSpec.hpp"
#include "antwika/ui/Theme.hpp"
#include "antwika/ui/WidgetId.hpp"

#include "Area.hpp"
#include "Caret.hpp"
#include "FocusRing.hpp"
#include "LayoutTree.hpp"
#include "Node.hpp"
#include "NodeKind.hpp"
#include "Saturate.hpp"
#include "TextEditing.hpp"

namespace antwika::ui
{

    // Every Node carries a std::string, as Context.cpp says.
    // That is all the GCOVR_EXCL_LINE markers below cover.

    namespace
    {
        using detail::Area;
        using detail::clampToU32;
        using detail::Editable;
        using detail::FocusRing;
        using detail::kNoNode;
        using detail::LayoutTree;
        using detail::Node;

        /**
         * @brief One line of a document, and what is marked on it.
         */
        struct Line
        {
            /** @brief The whole document the line is part of. */
            std::string_view text{};

            /** @brief Where this line starts in it. */
            std::size_t begin = 0;

            /** @brief Where it ends: its break, or the text's end. */
            std::size_t end = 0;

            /** @brief The selection's lower end, anywhere at all. */
            std::size_t low = 0;

            /** @brief Its higher end; equal to low when nothing is. */
            std::size_t high = 0;

            /** @brief The caret, when it is on this line. */
            std::optional<std::size_t> caret{};

            /** @brief The spans lit up, anywhere in the document. */
            std::span<const TextHighlight> lit{};
        };

        // Whether a whole piece sits inside some highlighted span.
        // Pieces are cut at every span's ends, so inside is all-or-none.
        [[nodiscard]] bool litPiece(
            const Line &line,
            const std::size_t from,
            const std::size_t to) noexcept
        {
            for (const auto &span : line.lit)
            {
                if (from >= span.begin && to <= span.end
                    && span.begin < span.end)
                {
                    return true;
                }
            }

            return false;
        }

        // One more than the breaks, since the last need not end in one.
        // A document with no break at all is one line.
        [[nodiscard]] std::size_t countLines(
            const std::string_view text) noexcept
        {
            return 1
                + static_cast<std::size_t>(
                       std::ranges::count(text, '\n'));
        }

        [[nodiscard]] std::uint32_t lineHeightOf(
            const Theme &theme) noexcept
        {
            return clampToU32(
                std::uint64_t{antwika::gfx::kGlyphLineHeight}
                * theme.textScale);
        }

        [[nodiscard]] std::uint32_t advanceOf(const Theme &theme) noexcept
        {
            return clampToU32(
                std::uint64_t{antwika::gfx::kGlyphAdvance}
                * theme.textScale);
        }

        /**
         * @brief Where one piece of a line is cut from the next.
         *
         * The line's own ends, whatever of the selection falls inside
         * it, and the caret -- sorted, and with cuts landing on the
         * same index counted once, so that no piece comes out empty
         * and every piece is either wholly selected or not at all.
         *
         * @param line The line and what is marked on it.
         * @return The cuts, in ascending order, at least one of them.
         */
        [[nodiscard]] std::vector<std::size_t> cutsIn(const Line &line)
        {
            std::vector<std::size_t> cuts{line.begin, line.end};

            if (line.low < line.high)
            {
                cuts.push_back(
                    std::clamp(line.low, line.begin, line.end));
                cuts.push_back(
                    std::clamp(line.high, line.begin, line.end));
            }

            if (line.caret)
            {
                cuts.push_back(*line.caret);
            }

            // A highlight cuts exactly as the selection does.
            // So every piece is wholly lit or not at all.
            for (const auto &span : line.lit)
            {
                if (span.begin < line.end && span.end > line.begin)
                {
                    cuts.push_back(
                        std::clamp(span.begin, line.begin, line.end));
                    cuts.push_back(
                        std::clamp(span.end, line.begin, line.end));
                }
            }

            std::ranges::sort(cuts);

            const auto repeated = std::ranges::unique(cuts);
            cuts.erase(repeated.begin(), repeated.end());

            return cuts;
            // Only an unwind destroys cuts at this brace.
        } // GCOVR_EXCL_LINE

        /**
         * @brief Append one line of a document as a row of pieces.
         *
         * A row rather than one text node, because a caret sits between
         * two pieces of a line and a selection puts its ground behind
         * one of them -- and antwika::gfx draws a run of characters in
         * one colour on nothing at all.
         *
         * @param tree The arena; a row is opened and closed on it.
         * @param theme The colours and metrics to draw from.
         * @param line The line and what is marked on it.
         */
        void addLine(
            LayoutTree &tree, const Theme &theme, const Line &line)
        {
            const auto height = lineHeightOf(theme);

            tree.open(Node{ // GCOVR_EXCL_LINE
                .axis = Axis::Row,
                .width = kGrow,
                .height = kFit,
                .gap = 0});

            // A blank line is still a line.
            // An empty text node measures nothing at all.
            // So every row is opened over a strut one glyph cell tall.
            tree.add(Node{ // GCOVR_EXCL_LINE
                .width = fixedSize(0),
                .height = fixedSize(height)});

            const auto cuts = cutsIn(line);

            for (std::size_t at = 0; at + 1 < cuts.size(); ++at)
            {
                if (cuts[at] == line.caret)
                {
                    tree.add(detail::caretNode(theme)); // GCOVR_EXCL_LINE
                }

                const auto from = cuts[at];
                const auto to = cuts[at + 1];

                // Whole pieces, which is what the cuts above are for.
                const bool picked = line.low < line.high && from >= line.low
                                    && to <= line.high;

                // The selection wins where the two overlap.
                // It is what the next keystroke acts on.
                const bool lit = !picked && litPiece(line, from, to);

                tree.add(Node{ // GCOVR_EXCL_LINE
                    .kind = detail::NodeKind::Text,
                    .width = kFit,
                    .height = kFit,
                    .background = picked
                        ? std::optional{theme.selection}
                        : (lit ? std::optional{theme.highlight}
                               : std::nullopt),
                    .text = std::string{ // GCOVR_EXCL_LINE
                        line.text.substr(from, to - from)},
                    .textScale = theme.textScale,
                    .textColor = theme.text});
            }

            // A caret at a line's end comes after every piece of it.
            // And is the whole of an empty line's row.
            if (line.caret == line.end)
            {
                tree.add(detail::caretNode(theme)); // GCOVR_EXCL_LINE
            }

            // A selection past this line's break shows on it.
            // As one cell of ground, on the break itself.
            // A blank line inside one would read as a hole otherwise.
            // A selection ends inside the text.
            // So a line it runs past has a break to draw this on.
            if (line.low < line.high && line.high > line.end)
            {
                tree.add(Node{ // GCOVR_EXCL_LINE
                    .width = fixedSize(advanceOf(theme)),
                    .height = fixedSize(height),
                    .background = theme.selection});
            }

            // What holds the text against a wide area's left edge.
            tree.add(Node{ // GCOVR_EXCL_LINE
                .axis = Axis::Row,
                .width = kGrow,
                .height = kFit});

            tree.close();
        }
    } // namespace

    void Context::textArea(const TextAreaSpec &spec)
    {
        // Past the end is the end, for both ends of a selection.
        // So a caller may hand an applied edit's indices straight back.
        const auto cursor = std::min(spec.cursor, spec.text.size());

        // An area told nothing about its far end selects nothing.
        // Which is that end being wherever the caret is.
        const auto anchor =
            std::min(spec.anchor.value_or(cursor), spec.text.size());

        // The spec's own flag overrides the focus this frame got.
        // The same rule a field and a button already follow.
        const bool focused =
            spec.focused
            || (spec.id != kNoWidget && spec.id == focusValue);

        if (focused)
        {
            pendingEdit = detail::editFor(
                Editable{
                    .id = spec.id,
                    .text = spec.text,
                    .cursor = cursor,
                    .anchor = anchor,
                    .multiline = true},
                keyboardValue);
        }

        const auto fill =
            focused ? themeValue.fieldFocused : themeValue.field;

        // An area is a stop in the tab order like a field is.
        const FocusRing ring{
            .color = themeValue.focusRing,
            .thickness = themeValue.focusRingThickness};

        tree->open(Node{ // GCOVR_EXCL_LINE
            .axis = Axis::Column,
            .width = spec.width,
            .height = spec.height,
            .padding = themeValue.buttonPadding,
            .gap = 0,
            .background = fill,
            .id = spec.id,
            .focusStyle = ring});

        const auto lines = countLines(spec.text);

        // As far down as the last line.
        // Which is as much as this can know without a layout.
        // How far it can usefully go is how many lines are showing.
        // resolve() answers that once there is a layout.
        // See Interactions::scrolled.
        // See Interactions::scrolled.
        const auto first = std::min(spec.scroll, lines - 1);

        // The lines and the bar sit side by side.
        // So a bar takes its width out of the room to write in.
        tree->open(Node{ // GCOVR_EXCL_LINE
            .axis = Axis::Row,
            .width = kGrow,
            .height = kGrow,
            .gap = 0});

        const auto column = tree->open(Node{ // GCOVR_EXCL_LINE
            .axis = Axis::Column,
            .width = kGrow,
            .height = kGrow,
            .gap = 0,
            .clips = true});

        // An unfocused area draws no caret.
        // It shows no selection for the same reason.
        // Neither is anything until the typing is going somewhere.
        const auto low = focused ? std::min(cursor, anchor) : 0;
        const auto high = focused ? std::max(cursor, anchor) : 0;

        // Ends past the text are brought inside, as the caret's is.
        // A span of nothing is dropped rather than cut around.
        std::vector<TextHighlight> clampedSpans;
        clampedSpans.reserve(spec.highlights.size());

        for (const auto &span : spec.highlights)
        {
            const TextHighlight inside{
                .begin = std::min(span.begin, spec.text.size()),
                .end = std::min(span.end, spec.text.size())};

            if (inside.begin < inside.end)
            {
                clampedSpans.push_back(inside);
            }
        }

        const std::span<const TextHighlight> clamped{clampedSpans};

        std::size_t begin = 0;

        for (std::size_t line = 0; line < lines; ++line)
        {
            const auto end = detail::endOfLine(spec.text, begin);

            // The caret sits between two pieces of one line.
            // And on exactly one line.
            // An index at a break belongs to the line it ends.
            const bool carries =
                focused && cursor >= begin && cursor <= end;

            if (line >= first)
            {
                addLine(
                    *tree,
                    themeValue,
                    Line{
                        .text = spec.text,
                        .begin = begin,
                        .end = end,
                        .low = low,
                        .high = high,
                        .caret = carries ? std::optional{cursor}
                                         : std::nullopt,
                        .lit = clamped});
            }

            begin = end + 1;
        }

        // Muted, and only while there is nothing of the caller's.
        // A placeholder is not content.
        if (spec.text.empty() && !spec.placeholder.empty())
        {
            label(spec.placeholder, themeValue.muted);
        }

        // What holds the lines against a tall area's top edge.
        spacer(kGrow);

        closeContainer();

        auto track = kNoNode;
        auto thumb = kNoNode;

        if (spec.scrollbar)
        {
            track = tree->open(Node{ // GCOVR_EXCL_LINE
                .axis = Axis::Column,
                .width = fixedSize(themeValue.scrollbarWidth),
                .height = kGrow,
                .gap = 0,
                .background = themeValue.scrollTrack});

            // Put where it goes by resolve() rather than by the layout.
            // Where it sits says how much is showing.
            // And how much that is only comes out of the arranged track.
            thumb = tree->add(Node{ // GCOVR_EXCL_LINE
                .width = kGrow,
                .height = fixedSize(0),
                .background = themeValue.scrollThumb});

            closeContainer();
        }

        closeContainer();
        closeContainer();

        tree->addArea(Area{
            .id = spec.id,
            .column = column,
            .track = track,
            .thumb = thumb,
            .text = spec.text,
            .scroll = first,
            .requested = spec.scroll,
            .lines = lines,
            .cursor = cursor,
            .anchor = anchor,
            // Never zero, so the arithmetic reading them divides by something.
            // A theme may ask for no scale at all.
            .lineHeight = std::max(1U, lineHeightOf(themeValue)),
            .advance = std::max(1U, advanceOf(themeValue)),
            .focused = focused});
    }

} // namespace antwika::ui
