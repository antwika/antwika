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
        using detail::getClampToU32;
        using detail::TextEditInput;
        using detail::FocusRing;
        using detail::kNoNode;
        using detail::LayoutTree;
        using detail::Node;

        struct Line final
        {
            std::string_view text{};

            std::size_t begin = 0;

            std::size_t end = 0;

            std::size_t lowIndex = 0;

            std::size_t highIndex = 0;

            std::optional<std::size_t> caret{};

            std::span<const TextHighlight> litHighlights{};
        };

        [[nodiscard]] bool isLitPiece(
            const Line &line,
            const std::size_t fromIndex,
            const std::size_t toIndex) noexcept
        {
            for (const auto &span : line.litHighlights)
            {
                if (fromIndex >= span.begin && toIndex <= span.end)
                {
                    return true;
                }
            }

            return false;
        }

        [[nodiscard]] std::size_t getCountLines(
            const std::string_view text) noexcept
        {
            return 1
                + static_cast<std::size_t>(
                       std::ranges::count(text, '\n'));
        }

        [[nodiscard]] std::uint32_t lineHeightOf(
            const Theme &theme) noexcept
        {
            return getClampToU32(
                std::uint64_t{
                    antwika::gfx::glyphLineHeightOf(theme.face)}
                * theme.textScale);
        }

        [[nodiscard]] std::uint32_t advanceOf(const Theme &theme) noexcept
        {
            return getClampToU32(
                std::uint64_t{antwika::gfx::glyphAdvanceOf(theme.face)}
                * theme.textScale);
        }

        [[nodiscard]] std::vector<std::size_t> cutsIn(const Line &line)
        {
            std::vector<std::size_t> cuts{line.begin, line.end};

            if (line.lowIndex < line.highIndex)
            {
                cuts.push_back(
                    std::clamp(line.lowIndex, line.begin, line.end));
                cuts.push_back(
                    std::clamp(line.highIndex, line.begin, line.end));
            }

            if (line.caret)
            {
                cuts.push_back(*line.caret);
            }

            for (const auto &span : line.litHighlights)
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

            const auto repeatedRange = std::ranges::unique(cuts);
            cuts.erase(repeatedRange.begin(), repeatedRange.end());

            return cuts;
        } // GCOVR_EXCL_LINE

        void addLine(
            LayoutTree &tree, const Theme &theme, const Line &line)
        {
            const auto height = lineHeightOf(theme);

            tree.open(Node{ // GCOVR_EXCL_LINE
                .axis = Axis::Row,
                .widthSizing = kGrowSizing,
                .heightSizing = getFixedSize(height),
                .gap = 0,
                .clips = true});

            const auto cuts = cutsIn(line);

            for (std::size_t index = 0; index + 1 < cuts.size(); ++index)
            {
                if (cuts[index] == line.caret)
                {
                    tree.add(detail::getCaretNode(theme)); // GCOVR_EXCL_LINE
                }

                const auto fromIndex = cuts[index];
                const auto toIndex = cuts[index + 1];

                const bool picked =
                    line.lowIndex < line.highIndex && fromIndex >= line.lowIndex
                                    && toIndex <= line.highIndex;

                const bool lit = !picked && isLitPiece(line, fromIndex, toIndex);

                tree.add(Node{ // GCOVR_EXCL_LINE
                    .kind = detail::NodeKind::Text,
                    .widthSizing = kFitSizing,
                    .heightSizing = kFitSizing,
                    .backgroundColor = picked
                                     ? std::optional{theme.selectionColor}
                        : (lit ? std::optional{theme.highlightColor}
                                     : std::nullopt),
                    .text = std::string{ // GCOVR_EXCL_LINE
                        line.text.substr(fromIndex, toIndex - fromIndex)},
                    .textScale = antwika::gfx::getEncodeTextScale(
                        theme.face, theme.textScale),
                    .textColor = theme.textColor});
            }

            if (line.caret == line.end)
            {
                tree.add(detail::getCaretNode(theme)); // GCOVR_EXCL_LINE
            }

            if (line.lowIndex < line.highIndex && line.highIndex > line.end)
            {
                tree.add(Node{ // GCOVR_EXCL_LINE
                    .widthSizing = getFixedSize(advanceOf(theme)),
                    .heightSizing = getFixedSize(height),
                    .backgroundColor = theme.selectionColor});
            }

            tree.add(Node{ // GCOVR_EXCL_LINE
                .axis = Axis::Row,
                .widthSizing = kGrowSizing,
                .heightSizing = kFitSizing});

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
            || (spec.widgetId != kNoWidget && spec.widgetId == focusedWidget);

        if (focused)
        {
            pendingEdit = detail::editFor(
                TextEditInput{
                    .widgetId = spec.widgetId,
                    .text = spec.text,
                    .cursor = cursor,
                    .anchor = anchor,
                    .multiline = true},
                keyboardValue);
        }

        const auto fill =
            focused ? themeValue.fieldFocusedColor : themeValue.fieldColor;

        const FocusRing ring{
            .color = themeValue.focusRingColor,
            .thickness = themeValue.focusRingThickness};

        tree->open(Node{ // GCOVR_EXCL_LINE
            .axis = Axis::Column,
            .widthSizing = spec.widthSizing,
            .heightSizing = spec.heightSizing,
            .padding = themeValue.buttonPadding,
            .gap = 0,
            .backgroundColor = fill,
            .widgetId = spec.widgetId,
            .focusStyle = ring});

        const auto lines = getCountLines(spec.text);

        const auto first = std::min(spec.scroll, lines - 1);

        tree->open(Node{ // GCOVR_EXCL_LINE
            .axis = Axis::Row,
            .widthSizing = kGrowSizing,
            .heightSizing = kGrowSizing,
            .gap = 0});

        const auto column = tree->open(Node{ // GCOVR_EXCL_LINE
            .axis = Axis::Column,
            .widthSizing = kGrowSizing,
            .heightSizing = kGrowSizing,
            .gap = 0,
            .clips = true});

        const auto lowIndex = focused ? std::min(cursor, anchor) : 0;
        const auto highIndex = focused ? std::max(cursor, anchor) : 0;

        std::vector<TextHighlight> clampedHighlights;
        clampedHighlights.reserve(spec.highlights.size());

        for (const auto &span : spec.highlights)
        {
            const TextHighlight insideHighlight{
                .begin = std::min(span.begin, spec.text.size()),
                .end = std::min(span.end, spec.text.size())};

            if (insideHighlight.begin < insideHighlight.end)
            {
                clampedHighlights.push_back(insideHighlight);
            }
        }

        const std::span<const TextHighlight> highlightSpan{clampedHighlights};

        std::size_t begin = 0;

        for (std::size_t line = 0; line < lines; ++line)
        {
            const auto end = detail::getEndOfLine(spec.text, begin);

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
                        .lowIndex = lowIndex,
                        .highIndex = highIndex,
                        .caret = carries ? std::optional{cursor}
                               : std::nullopt,
                        .litHighlights = highlightSpan});

                for (const auto &band : spec.bandRuns)
                {
                    if (band.line != line || band.rows == 0)
                    {
                        continue;
                    }

                    tree->add(Node{ // GCOVR_EXCL_LINE
                        .widthSizing = kGrowSizing,
                        .heightSizing = getFixedSize(getClampToU32(
                            std::uint64_t{band.rows}
                            * lineHeightOf(themeValue))),
                        .widgetId = band.widgetId});
                }
            }

            begin = end + 1;
        }

        if (spec.text.empty() && !spec.placeholder.empty())
        {
            label(spec.placeholder, themeValue.mutedColor);
        }

        spacer(kGrowSizing);

        closeContainer();

        auto track = kNoNode;
        auto thumb = kNoNode;

        if (spec.scrollbar)
        {
            track = tree->open(Node{ // GCOVR_EXCL_LINE
                .axis = Axis::Column,
                .widthSizing = getFixedSize(themeValue.scrollbarWidth),
                .heightSizing = kGrowSizing,
                .gap = 0,
                .backgroundColor = themeValue.scrollTrackColor});

            thumb = tree->add(Node{ // GCOVR_EXCL_LINE
                .widthSizing = kGrowSizing,
                .heightSizing = getFixedSize(0),
                .backgroundColor = themeValue.scrollThumbColor});

            closeContainer();
        }

        closeContainer();
        closeContainer();

        tree->addArea(Area{
            .widgetId = spec.widgetId,
            .column = column,
            .track = track,
            .thumb = thumb,
            .text = spec.text,
            .scroll = first,
            .requestedExtent = spec.scroll,
            .lines = lines,
            .cursor = cursor,
            .anchor = anchor,
            .dragging = spec.dragging,
            .bandRuns = spec.bandRuns,
            .lineHeight = std::max(1U, lineHeightOf(themeValue)),
            .advance = std::max(1U, advanceOf(themeValue)),
            .focused = focused});
    }

}
