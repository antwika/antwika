#pragma once

#include <optional>

#include "antwika/ui/OptionChoice.hpp"
#include "antwika/ui/ScrollChange.hpp"
#include "antwika/ui/SliderChange.hpp"
#include "antwika/ui/SplitChange.hpp"
#include "antwika/ui/TextAreaSpec.hpp"
#include "antwika/ui/TextEdit.hpp"
#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui
{

    struct AreaPress final
    {
        WidgetId area = kNoWidget;

        DragHome home = DragHome::None;

        [[nodiscard]] bool operator==(const AreaPress &other) const =
            default;
    };

    struct Interactions final
    {
        WidgetId hovered = kNoWidget;

        WidgetId activated = kNoWidget;

        WidgetId focused = kNoWidget;

        bool pointerOverUi = false;

        std::optional<TextEdit> edit{};

        std::optional<OptionChoice> chosen{};

        std::optional<ScrollChange> scrolled{};

        std::optional<AreaPress> areaPress{};

        std::optional<SliderChange> slid{};

        std::optional<SplitChange> split{};

        [[nodiscard]] bool operator==(const Interactions &other) const =
            default;
    };

}
