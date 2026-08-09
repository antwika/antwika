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

    namespace
    {
        using detail::Editable;
        using detail::FocusRing;
        using detail::Node;
    }

    void Context::textField(const TextFieldSpec &spec)
    {
        const auto cursor = std::min(spec.cursor, spec.text.size());

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
                    .anchor = cursor},
                keyboardValue);
        }

        const auto fill =
            focused ? themeValue.fieldFocused : themeValue.field;

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

        if (spec.text.empty() && !spec.placeholder.empty())
        {
            label(spec.placeholder, themeValue.muted);
        }

        spacer(kGrow);

        closeContainer();
    }

}
