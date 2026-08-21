#pragma once

#include "antwika/ui/DragOrigin.hpp"
#include "antwika/ui/OptionChoice.hpp"
#include "antwika/ui/ScrollChange.hpp"
#include "antwika/ui/SliderChange.hpp"
#include "antwika/ui/SplitChange.hpp"
#include "antwika/ui/TextAreaSpec.hpp"
#include "antwika/ui/TextEdit.hpp"
#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui
{

    struct TextAreaPress final
    {
        WidgetId areaWidget = kNoWidget;

        DragOrigin homeOrigin = DragOrigin::None;

        [[nodiscard]] bool operator==(const TextAreaPress &other) const =
            default;
    };

}
