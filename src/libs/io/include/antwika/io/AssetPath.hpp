#pragma once

#include <string>
#include <string_view>

namespace antwika::io
{

    [[nodiscard]] std::string getExecutableDirectory();

    [[nodiscard]] std::string getAssetPath(std::string_view name);

}
