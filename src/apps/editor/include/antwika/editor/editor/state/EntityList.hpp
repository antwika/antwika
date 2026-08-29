#pragma once

#include <cstddef>
#include <vector>

#include "antwika/editor/editor/state/EntityRow.hpp"

namespace antwika::editor
{

    struct EntityList final
    {
        std::vector<EntityRow> rows;

        std::size_t scrollLine = 0;

        bool trackHeld = false;
    };

}
