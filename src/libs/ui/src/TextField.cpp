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
#include "antwika/ui/Keyboard.hpp"
#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/TextEdit.hpp"
#include "antwika/ui/TextFieldSpec.hpp"
#include "antwika/ui/WidgetId.hpp"

#include "FocusRing.hpp"
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
        using detail::FocusRing;
        using detail::Node;

        /**
         * @brief Work out what this frame's typing came to.
         *
         * Applied to a copy of the caller's characters, never to the
         * caller's own: this library holds nothing between frames, so
         * what it can offer is the answer, not the edit.
         *
         * The characters go in first and the keys are read after, in
         * the order they arrived.
         * A character is not an edge with a meaning of its own.
         * So there is nothing for it to be interleaved with.
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
            const Keyboard &keys)
        {
            TextEdit edit{
                .field = spec.id,
                .text = std::string{spec.text}, // GCOVR_EXCL_LINE
                .cursor = cursor}; // GCOVR_EXCL_LINE

            bool moved = false;

            if (!keys.typed.empty())
            {
                edit.text.insert(edit.cursor, keys.typed);
                edit.cursor += keys.typed.size();
                moved = true;
            }

            for (const auto key : keys.keys)
            {
                // Backspace at the start has nothing to take.
                if (key == Key::Backspace && edit.cursor > 0)
                {
                    edit.text.erase(edit.cursor - 1, 1);
                    --edit.cursor;
                    moved = true;
                }

                if (key == Key::MoveLeft && edit.cursor > 0)
                {
                    --edit.cursor;
                    moved = true;
                }

                if (key == Key::MoveRight
                    && edit.cursor < edit.text.size())
                {
                    ++edit.cursor;
                    moved = true;
                }

                // The same edge resolve() activates the field on.
                if (key == Key::Activate)
                {
                    edit.submitted = true;
                }

                if (key == Key::Cancel)
                {
                    edit.cancelled = true;
                }

                // A focus key is focus's alone, and moves no caret.
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

        // The spec's own flag overrides the focus this frame got.
        // Focus moves inside finish().
        // So a field describing itself knows where focus started.
        // That is the rule a button's activation already follows.
        const bool focused =
            spec.focused
            || (spec.id != kNoWidget && spec.id == focusValue);

        if (focused)
        {
            pendingEdit = editFor(spec, cursor, keyboardValue);
        }

        const auto fill =
            focused ? themeValue.fieldFocused : themeValue.field;

        // A field is a stop in the tab order like a button is.
        const FocusRing ring{
            .color = themeValue.focusRing,
            .thickness = themeValue.focusRingThickness};

        tree->open(Node{ // GCOVR_EXCL_LINE
            .axis = Axis::Row,
            .width = spec.width,
            .height = kFit,
            .cross = Alignment::Center,
            .padding = themeValue.buttonPadding,
            .gap = 0,
            .background = fill,
            .id = spec.id,
            .focusStyle = ring});

        // The caret sits between two pieces of one line.
        // So a row of three children is the whole of it.
        // Nothing has to be positioned by hand.
        const auto head = spec.text.substr(0, cursor);
        const auto tail = spec.text.substr(cursor);

        if (!head.empty())
        {
            label(head, themeValue.text);
        }

        if (focused)
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
