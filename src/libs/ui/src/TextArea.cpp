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

    namespace
    {
        using detail::Area;
        using detail::clampToU32;
        using detail::Editable;
        using detail::FocusRing;
        using detail::kNoNode;
        using detail::LayoutTree;
        using detail::Node;

        struct Line final
        {
            std::string_view text{};

            std::size_t begin = 0;

            std::size_t end = 0;

            std::size_t low = 0;

            std::size_t high = 0;

            std::optional<std::size_t> caret{};

            std::span<const TextHighlight> lit{};
        };

        [[nodiscard]] bool litPiece(
            const Line &line,
            const std::size_t from,
            const std::size_t to) noexcept
        {
            for (const auto &span : line.lit)
            {
                if (from >= span.begin && to <= span.end)
                {
                    return true;
                }
            }

            return false;
        }

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
        } // GCOVR_EXCL_LINE

        void addLine(
            LayoutTree &tree, const Theme &theme, const Line &line)
        {
            const auto height = lineHeightOf(theme);

            tree.open(Node{ // GCOVR_EXCL_LINE
                .axis = Axis::Row,
                .width = kGrow,
                .height = fixedSize(height),
                .gap = 0,
                .clips = true});

            const auto cuts = cutsIn(line);

            for (std::size_t at = 0; at + 1 < cuts.size(); ++at)
            {
                if (cuts[at] == line.caret)
                {
                    tree.add(detail::caretNode(theme)); // GCOVR_EXCL_LINE
                }

                const auto from = cuts[at];
                const auto to = cuts[at + 1];

                const bool picked = line.low < line.high && from >= line.low
                                    && to <= line.high;

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

            if (line.caret == line.end)
            {
                tree.add(detail::caretNode(theme)); // GCOVR_EXCL_LINE
            }

            if (line.low < line.high && line.high > line.end)
            {
                tree.add(Node{ // GCOVR_EXCL_LINE
                    .width = fixedSize(advanceOf(theme)),
                    .height = fixedSize(height),
                    .background = theme.selection});
            }

            tree.add(Node{ // GCOVR_EXCL_LINE
                .axis = Axis::Row,
                .width = kGrow,
                .height = kFit});

            tree.close();
        }
    }

    void Context::textArea(const TextAreaSpec &spec)
    {
        const auto cursor = std::min(spec.cursor, spec.text.size());

        const auto anchor =
            std::min(spec.anchor.value_or(cursor), spec.text.size());

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

        const auto first = std::min(spec.scroll, lines - 1);

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

        const auto low = focused ? std::min(cursor, anchor) : 0;
        const auto high = focused ? std::max(cursor, anchor) : 0;

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

                for (const auto &band : spec.bands)
                {
                    if (band.line != line || band.rows == 0)
                    {
                        continue;
                    }

                    tree->add(Node{ // GCOVR_EXCL_LINE
                        .width = kGrow,
                        .height = fixedSize(clampToU32(
                            std::uint64_t{band.rows}
                            * lineHeightOf(themeValue))),
                        .id = band.id});
                }
            }

            begin = end + 1;
        }

        if (spec.text.empty() && !spec.placeholder.empty())
        {
            label(spec.placeholder, themeValue.muted);
        }

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
            .dragging = spec.dragging,
            .bands = spec.bands,
            .lineHeight = std::max(1U, lineHeightOf(themeValue)),
            .advance = std::max(1U, advanceOf(themeValue)),
            .focused = focused});
    }

}
