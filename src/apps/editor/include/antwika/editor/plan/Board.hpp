#pragma once

#include <array>
#include <vector>
#include <antwika/gfx/Rect.hpp>
#include <antwika/ui/WidgetId.hpp>
#include "antwika/editor/plan/Card.hpp"

namespace antwika::editor
{

    struct Board final
    {
        std::array<std::vector<Card>, 3> columnCards{};

        [[nodiscard]] bool operator==(const Board &other) const
            = default;
    };

}
