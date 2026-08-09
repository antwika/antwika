#pragma once

#include <string_view>

namespace antwika::cli
{

    struct FlagSpec final
    {
        std::string_view name{};

        std::string_view valueName{};

        std::string_view help{};
    };

}
