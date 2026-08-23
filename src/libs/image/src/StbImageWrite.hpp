#pragma once

#include <string>

namespace antwika::image::detail
{

    [[nodiscard]] std::string getEncodePng(
        const unsigned char *pixels, int width, int height, int channels);

}
