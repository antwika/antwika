#pragma once

#include <string>
#include <string_view>

namespace antwika::io
{

    [[nodiscard]] std::string executableDirectory();

    [[nodiscard]] std::string assetPath(std::string_view name);

}
