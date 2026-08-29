#pragma once

#include <string_view>

namespace antwika::editor
{

    struct SheetNames final
    {
        std::string_view sheetName;

        std::string_view drawName;

        [[nodiscard]] bool operator==(const SheetNames &other) const =
            default;
    };

}
