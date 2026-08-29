#include "antwika/render/Checkerboard.hpp"

#include <algorithm>
#include <cstddef>

#include <antwika/gfx/Color.hpp>

namespace antwika::render
{

    gfx::Bitmap getCheckerboardBitmap(
        const antwika::gfx::Size size, const std::uint32_t check)
    {
        constexpr antwika::gfx::Color kDimColor{
            .red = 62, .green = 64, .blue = 78, .alpha = 255};
        constexpr antwika::gfx::Color kLitColor{
            .red = 78, .green = 80, .blue = 96, .alpha = 255};

        const auto checkSpan = std::max<std::uint32_t>(check, 1U);

        antwika::gfx::Bitmap bitmap{.size = size, .pixels = {}};

        bitmap.pixels.reserve(
            static_cast<std::size_t>(size.width)
            * size.height * antwika::gfx::kBytesPerPixel);

        for (std::uint32_t row = 0; row < size.height; ++row)
        {
            for (std::uint32_t column = 0;
                 column < size.width;
                 ++column)
            {
                const auto color =
                    ((row / checkSpan) + (column / checkSpan)) % 2 == 0
                        ? kDimColor
                        : kLitColor;

                bitmap.pixels.push_back(color.red);
                bitmap.pixels.push_back(color.green);
                bitmap.pixels.push_back(color.blue);
                bitmap.pixels.push_back(color.alpha);
            }
        }

        return bitmap;
    } // GCOVR_EXCL_LINE

}
