#pragma once

#include <string>

namespace antwika::gfx::detail
{

    [[nodiscard]] std::string encodePng(
        const unsigned char *pixels, int width, int height, int channels);

}
