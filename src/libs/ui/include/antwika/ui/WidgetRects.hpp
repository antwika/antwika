#pragma once

#include <optional>
#include <vector>

#include <antwika/gfx/Rect.hpp>

#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui
{

    using antwika::gfx::Rect;

    struct WidgetRect final
    {
        WidgetId id = kNoWidget;

        Rect rect{};

        [[nodiscard]] bool operator==(const WidgetRect &other) const =
            default;
    };

    struct WidgetRects final
    {
        std::vector<WidgetRect> entries{};

        [[nodiscard]] std::optional<Rect> find(WidgetId id) const;

        [[nodiscard]] bool operator==(const WidgetRects &other) const =
            default;
    };

}
