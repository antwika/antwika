#include "antwika/gfx/Bitmap.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>

namespace antwika::gfx
{

    namespace
    {
        [[nodiscard]] std::optional<std::size_t> byteAt(
            const Bitmap &bitmap,
            const std::int32_t x,
            const std::int32_t y)
        {
            if (x < 0 || y < 0
                || static_cast<std::uint32_t>(x)
                       >= bitmap.size.width
                || static_cast<std::uint32_t>(y)
                       >= bitmap.size.height)
            {
                return std::nullopt;
            }

            const auto byteIndex =
                ((static_cast<std::size_t>(y)
                  * static_cast<std::size_t>(bitmap.size.width))
                 + static_cast<std::size_t>(x))
                * kBytesPerPixel;

            if (byteIndex + kBytesPerPixel > bitmap.pixels.size())
            {
                return std::nullopt;
            }

            return byteIndex;
        }
    }

    bool Bitmap::isValid() const noexcept
    {
        if (size.width == 0 || size.height == 0)
        {
            return false;
        }

        const std::size_t expectedBytes = static_cast<std::size_t>(size.width)
            * static_cast<std::size_t>(size.height) * kBytesPerPixel;

        return pixels.size() == expectedBytes;
    }

    std::optional<Color> colorAt(
        const Bitmap &bitmap,
        const std::int32_t x,
        const std::int32_t y)
    {
        const auto byteIndex = byteAt(bitmap, x, y);

        if (!byteIndex.has_value())
        {
            return std::nullopt;
        }

        return Color{
            .red = bitmap.pixels[*byteIndex],
            .green = bitmap.pixels[*byteIndex + 1],
            .blue = bitmap.pixels[*byteIndex + 2],
            .alpha = bitmap.pixels[*byteIndex + 3]};
    }

    void setColorAt(
        Bitmap &bitmap,
        const std::int32_t x,
        const std::int32_t y,
        const Color color)
    {
        const auto byteIndex = byteAt(bitmap, x, y);

        if (!byteIndex.has_value())
        {
            return;
        }

        bitmap.pixels[*byteIndex] = color.red;
        bitmap.pixels[*byteIndex + 1] = color.green;
        bitmap.pixels[*byteIndex + 2] = color.blue;
        bitmap.pixels[*byteIndex + 3] = color.alpha;
    }

}
