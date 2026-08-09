#pragma once

#include <cstdint>
#include <optional>

#include "antwika/ui/Interactions.hpp"
#include "antwika/ui/Keyboard.hpp"
#include "antwika/ui/Pointer.hpp"
#include "antwika/ui/TextEdit.hpp"
#include "antwika/ui/Theme.hpp"
#include "antwika/ui/WidgetId.hpp"

#include "LayoutTree.hpp"

namespace antwika::ui::detail
{

    Interactions resolve(
        LayoutTree &tree,
        const Pointer &pointer,
        const Keyboard &keyboard,
        WidgetId focus,
        std::optional<TextEdit> &edit,
        std::uint32_t thumbWidth);

    inline Interactions resolve(
        LayoutTree &tree,
        const Pointer &pointer,
        const Keyboard &keyboard = {},
        WidgetId focus = kNoWidget)
    {
        std::optional<TextEdit> edit;

        return resolve(
            tree, pointer, keyboard, focus, edit,
            Theme{}.sliderThumbWidth);
    }

}
