#include <algorithm>

#include "antwika/ui/Alignment.hpp"
#include "antwika/ui/Axis.hpp"
#include "antwika/ui/Context.hpp"
#include "antwika/ui/Keyboard.hpp"
#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/TextEdit.hpp"
#include "antwika/ui/TextFieldSpec.hpp"
#include "antwika/ui/WidgetId.hpp"

#include "Caret.hpp"
#include "FocusRing.hpp"
#include "LayoutTree.hpp"
#include "Node.hpp"
#include "TextEditing.hpp"

namespace antwika::ui
{

    // Every Node carries a std::string, as Context.cpp says.
    // That is all the GCOVR_EXCL_LINE markers below cover.

    namespace
    {
        using detail::Editable;
        using detail::FocusRing;
        using detail::Node;
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
            pendingEdit = detail::editFor(
                // Both ends together.
                // A field draws no selection, so it is handed none.
                Editable{
                    .id = spec.id,
                    .text = spec.text,
                    .cursor = cursor,
                    .anchor = cursor},
                keyboardValue);
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
            tree->add(detail::caretNode(themeValue)); // GCOVR_EXCL_LINE
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
