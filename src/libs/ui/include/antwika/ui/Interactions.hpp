#pragma once

#include <optional>

#include "antwika/ui/EdgeChange.hpp"
#include "antwika/ui/OptionChoice.hpp"
#include "antwika/ui/ScrollChange.hpp"
#include "antwika/ui/SliderChange.hpp"
#include "antwika/ui/SplitChange.hpp"
#include "antwika/ui/TextAreaPress.hpp"
#include "antwika/ui/TextAreaSpec.hpp"
#include "antwika/ui/TextEdit.hpp"
#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui
{

    struct Interactions final
    {
        WidgetId hoveredWidget = kNoWidget;

        WidgetId activatedWidget = kNoWidget;

        WidgetId focusedWidget = kNoWidget;

        bool pointerOverUi = false;

        std::optional<TextEdit> edit{};

        std::optional<OptionChoice> chosenChoice{};

        std::optional<ScrollChange> scrollChange{};

        std::optional<TextAreaPress> areaPress{};

        std::optional<SliderChange> slidChange{};

        std::optional<SplitChange> split{};

        std::optional<EdgeChange> edge{};

        [[nodiscard]] bool operator==(const Interactions &other) const =
            default;
    };

}
