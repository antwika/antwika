#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include <antwika/gfx/Glyphs.hpp>

#include "antwika/ui/Alignment.hpp"
#include "antwika/ui/Axis.hpp"
#include "antwika/ui/Context.hpp"
#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/TextEdit.hpp"
#include "antwika/ui/TextFieldSpec.hpp"
#include "antwika/ui/TextInput.hpp"

#include "LayoutTree.hpp"
#include "Node.hpp"
#include "Saturate.hpp"

namespace antwika::ui
{

    // Every Node carries a std::string, as Context.cpp says.
    // That is all the GCOVR_EXCL_LINE markers below cover.

    namespace
    {
        using detail::clampToU32;
        using detail::Node;

        /**
         * @brief Work out what this frame's typing came to.
         *
         * Applied to a copy of the caller's characters, never to the
         * caller's own: this library holds nothing between frames, so
         * what it can offer is the answer, not the edit.
         *
         * @param spec The field being typed into.
         * @param cursor The caret, already brought inside the text.
         * @param keys What arrived this frame.
         * @return The edit, or nothing when this frame left the field
         * exactly as it was.
         */
        std::optional<TextEdit> editFor(
            const TextFieldSpec &spec,
            std::size_t cursor,
            const TextInput &keys)
        {
            TextEdit edit{
                .field = spec.id,
                .text = std::string{spec.text}, // GCOVR_EXCL_LINE
                .cursor = cursor,
                .submitted = keys.submit,
                .cancelled = keys.cancel};

            bool moved = false;

            if (!keys.typed.empty())
            {
                edit.text.insert(edit.cursor, keys.typed);
                edit.cursor += keys.typed.size();
                moved = true;
            }

            // Backspace at the start has nothing before it to take.
            if (keys.backspace && edit.cursor > 0)
            {
                edit.text.erase(edit.cursor - 1, 1);
                --edit.cursor;
                moved = true;
            }

            if (keys.left && edit.cursor > 0)
            {
                --edit.cursor;
                moved = true;
            }

            if (keys.right && edit.cursor < edit.text.size())
            {
                ++edit.cursor;
                moved = true;
            }

            if (!moved && !edit.submitted && !edit.cancelled)
            {
                return {};
            }

            return edit;
        }
    } // namespace

    void Context::textField(const TextFieldSpec &spec)
    {
        // Past the end is the end.
        // So a caller may hand an applied edit's cursor straight back.
        const auto cursor = std::min(spec.cursor, spec.text.size());

        if (spec.focused)
        {
            pendingEdit = editFor(spec, cursor, keysValue);
        }

        const auto fill =
            spec.focused ? themeValue.fieldFocused : themeValue.field;

        tree->open(Node{ // GCOVR_EXCL_LINE
            .axis = Axis::Row,
            .width = spec.width,
            .height = kFit,
            .cross = Alignment::Center,
            .padding = themeValue.buttonPadding,
            .gap = 0,
            .background = fill,
            .id = spec.id});

        // The caret sits between two pieces of one line.
        // So a row of three children is the whole of it.
        // Nothing has to be positioned by hand.
        const auto head = spec.text.substr(0, cursor);
        const auto tail = spec.text.substr(cursor);

        if (!head.empty())
        {
            label(head, themeValue.text);
        }

        if (spec.focused)
        {
            const auto height = clampToU32(
                std::uint64_t{antwika::gfx::kGlyphLineHeight}
                * themeValue.textScale);
            const auto width =
                themeValue.textScale > 0 ? themeValue.textScale : 1;

            tree->add(Node{ // GCOVR_EXCL_LINE
                .width = fixedSize(width),
                .height = fixedSize(height),
                .background = themeValue.caret});
        }

        if (!tail.empty())
        {
            label(tail, themeValue.text);
        }

        // Muted, and only while there is nothing of the caller's.
        // A placeholder is not content.
        if (spec.text.empty() && !spec.placeholder.empty())
        {
            label(spec.placeholder, themeValue.muted);
        }

        // What holds the text against a wide field's left edge.
        spacer(kGrow);

        closeContainer();
    }

} // namespace antwika::ui
