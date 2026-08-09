#include "antwika/gfx/PngReader.hpp"

#include <cstdint>
#include <istream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include "antwika/gfx/Bitmap.hpp"
#include "antwika/gfx/GfxError.hpp"

#include "StbImage.hpp"

namespace antwika::gfx
{

    namespace
    {
        struct PixelDeleter final
        {
            void operator()(unsigned char *pixels) const noexcept
            {
                detail::freeDecodedImage(pixels);
            }
        };

        using OwnedPixels = std::unique_ptr<unsigned char, PixelDeleter>;
    }

    Bitmap PngReader::read(std::istream &in) const
    {
        const std::vector<unsigned char> bytes{
            std::istreambuf_iterator<char>(in),
            std::istreambuf_iterator<char>()};

        const auto decoded = detail::decodeImage(
            bytes.data(),
            static_cast<int>(bytes.size()),
            static_cast<int>(kBytesPerPixel));

        const OwnedPixels pixels{decoded.pixels};

        if (pixels == nullptr)
        {
            throw GfxError(
                std::string("gfx: could not decode a PNG: ")
                + detail::decodeFailureReason());
        }

        const auto count = static_cast<std::size_t>(decoded.width)
            * static_cast<std::size_t>(decoded.height) * kBytesPerPixel;

        return Bitmap{
            .size = {
                .width = static_cast<std::uint32_t>(decoded.width),
                .height = static_cast<std::uint32_t>(decoded.height)},
            .pixels = std::vector<std::uint8_t>(
                pixels.get(), pixels.get() + count)};
    }

}
