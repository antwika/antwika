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
        using detail::TextEditInput;
        using detail::FocusRing;
        using detail::Node;
    }

    void Context::textField(const TextFieldSpec &spec)
    {
        const auto cursor = std::min(spec.cursor, spec.text.size());

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
                    .anchor = cursor},
                keyboardValue);
        }

        const auto fill =
            focused ? themeValue.fieldFocusedColor : themeValue.fieldColor;

        const FocusRing ring{
            .color = themeValue.focusRingColor,
            .thickness = themeValue.focusRingThickness};

        tree->open(Node{ // GCOVR_EXCL_LINE
            .axis = Axis::Row,
            .widthSizing = spec.widthSizing,
            .heightSizing = kFitSizing,
            .crossAlignment = Alignment::Center,
            .padding = themeValue.buttonPadding,
            .gap = 0,
            .backgroundColor = fill,
            .widgetId = spec.widgetId,
            .focusStyle = ring});

        const auto head = spec.text.substr(0, cursor);
        const auto tail = spec.text.substr(cursor);

        if (!head.empty())
        {
            label(head, themeValue.textColor);
        }

        if (focused)
        {
            tree->add(detail::caretNode(themeValue)); // GCOVR_EXCL_LINE
        }

        if (!tail.empty())
        {
            label(tail, themeValue.textColor);
        }

        if (spec.text.empty() && !spec.placeholder.empty())
        {
            label(spec.placeholder, themeValue.mutedColor);
        }

        spacer(kGrowSizing);

        closeContainer();
    }

}
