#pragma once

#include <cstddef>
#include <optional>
#include "antwika/editor/plan/PlanBoard.hpp"

namespace antwika::editor
{

    struct PlanDrag final
    {
        Column fromColumn = Column::Todo;

        std::size_t cardIndex = 0;

        gfx::Point grabbedAtPoint{};

        bool moved = false;

        std::optional<Column> overColumn;

        std::size_t dropIndex = 0;
    };

}
