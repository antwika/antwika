#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include <antwika/gfx/Glyphs.hpp>

#include "antwika/ui/Axis.hpp"
#include "antwika/ui/Context.hpp"
#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/TextAreaSpec.hpp"
#include "antwika/ui/WidgetId.hpp"

#include "Caret.hpp"
#include "FocusRing.hpp"
#include "LayoutTree.hpp"
#include "Node.hpp"
#include "Saturate.hpp"
#include "TextEditing.hpp"

namespace antwika::ui
{

    // Every Node carries a std::string, as Context.cpp says.
    // That is all the GCOVR_EXCL_LINE markers below cover.

    namespace
    {
        using detail::clampToU32;
        using detail::Editable;
        using detail::FocusRing;
        using detail::Node;
    } // namespace

    void Context::textArea(const TextAreaSpec &spec)
    {
        // Past the end is the end.
        // So a caller may hand an applied edit's cursor straight back.
        const auto cursor = std::min(spec.cursor, spec.text.size());

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

        // A blank line is still a line.
        // An empty text node measures nothing at all.
        // So every row is opened over a strut one glyph cell tall.
        const auto lineHeight = clampToU32(
            std::uint64_t{antwika::gfx::kGlyphLineHeight}
            * themeValue.textScale);

        std::size_t begin = 0;

        while (true)
        {
            const auto end = detail::endOfLine(spec.text, begin);
            const auto line = spec.text.substr(begin, end - begin);

            {
                tree->open(Node{ // GCOVR_EXCL_LINE
                    .axis = Axis::Row,
                    .width = kGrow,
                    .height = kFit,
                    .gap = 0});

                tree->add(Node{ // GCOVR_EXCL_LINE
                    .width = fixedSize(0),
                    .height = fixedSize(lineHeight)});

                // The caret sits between two pieces of one line.
                // And on exactly one line.
                // An index at a break belongs to the line it ends.
                const bool carries =
                    focused && cursor >= begin && cursor <= end;

                const auto split = carries ? cursor - begin : line.size();

                const auto head = line.substr(0, split);
                const auto tail = line.substr(split);

                if (!head.empty())
                {
                    label(head, themeValue.text);
                }

                if (carries)
                {
                    tree->add( // GCOVR_EXCL_LINE
                        detail::caretNode(themeValue));
                }

                if (!tail.empty())
                {
                    label(tail, themeValue.text);
                }

                // What holds the text against a wide area's left edge.
                spacer(kGrow);

                closeContainer();
            }

            if (end == spec.text.size())
            {
                break;
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
    }

} // namespace antwika::ui
