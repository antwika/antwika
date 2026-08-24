#pragma once

#include <cstddef>
#include <optional>
#include <string>

namespace antwika::editor
{

    struct FigureTool final
    {
        std::optional<std::size_t> chosenIndex;

        bool placed = false;

        std::string pendingLine;
    };

}
