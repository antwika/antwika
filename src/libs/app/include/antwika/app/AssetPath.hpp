#pragma once

#include <string>
#include <string_view>

namespace antwika::app
{

    [[nodiscard]] std::string executableDirectory();

    [[nodiscard]] std::string assetPath(std::string_view name);

}
