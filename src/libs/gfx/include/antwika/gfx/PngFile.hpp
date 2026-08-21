#pragma once

#include <string>
#include <string_view>

#include "antwika/gfx/Bitmap.hpp"

namespace antwika::gfx
{

    [[nodiscard]] Bitmap readPngFile(
        const std::string &path, std::string_view name);

    void writePngFile(
        const Bitmap &bitmap,
        const std::string &path,
        std::string_view name);

}
