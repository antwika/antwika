#include "antwika/atlas_editor/Canvas.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "antwika/atlas_editor/AtlasEditorError.hpp"

namespace antwika::atlas_editor
{

    namespace
    {
        // Where a pixel's four bytes begin.
        // Only ever called for a pixel holds() has admitted.
        // So the widening is enough and nothing needs clamping.
        [[nodiscard]] std::size_t offsetOf(
            const Size size, const Pixel pixel) noexcept
        {
            return (static_cast<std::size_t>(pixel.y)
                        * static_cast<std::size_t>(size.width)
                    + static_cast<std::size_t>(pixel.x))
                   * antwika::gfx::kBytesPerPixel;
        }
    } // namespace

    Canvas::Canvas(Bitmap image, const std::uint64_t revision)
        : image(std::move(image)), changes(revision)
    {
        if (!this->image.isComplete())
        {
            throw AtlasEditorError(
                "atlas_editor: the image does not hold the pixels it "
                "claims to");
        }
    }

    Canvas Canvas::blank(const Size size)
    {
        // Built here and then handed to the constructor to check.
        // What a complete bitmap is stays stated in one place.
        // A size this refuses is one isComplete() would refuse too.
        Bitmap empty{
            .size = size,
            .pixels = std::vector<std::uint8_t>(
                static_cast<std::size_t>(size.width)
                    * static_cast<std::size_t>(size.height)
                    * antwika::gfx::kBytesPerPixel,
                0)};

        return Canvas(std::move(empty));
    }

    Size Canvas::size() const noexcept
    {
        return image.size;
    }

    bool Canvas::holds(const Pixel pixel) const noexcept
    {
        return pixel.x >= 0 && pixel.y >= 0
               && static_cast<std::uint32_t>(pixel.x) < image.size.width
               && static_cast<std::uint32_t>(pixel.y) < image.size.height;
    }

    Color Canvas::at(const Pixel pixel) const noexcept
    {
        if (!holds(pixel))
        {
            return Color{.red = 0, .green = 0, .blue = 0, .alpha = 0};
        }

        const std::size_t offset = offsetOf(image.size, pixel);

        return Color{
            .red = image.pixels[offset],
            .green = image.pixels[offset + 1],
            .blue = image.pixels[offset + 2],
            .alpha = image.pixels[offset + 3]};
    }

    bool Canvas::set(const Pixel pixel, const Color color) noexcept
    {
        if (!holds(pixel) || at(pixel) == color)
        {
            return false;
        }

        const std::size_t offset = offsetOf(image.size, pixel);

        image.pixels[offset] = color.red;
        image.pixels[offset + 1] = color.green;
        image.pixels[offset + 2] = color.blue;
        image.pixels[offset + 3] = color.alpha;

        ++changes;

        return true;
    }

    const Bitmap &Canvas::bitmap() const noexcept
    {
        return image;
    }

    std::uint64_t Canvas::revision() const noexcept
    {
        return changes;
    }

} // namespace antwika::atlas_editor
