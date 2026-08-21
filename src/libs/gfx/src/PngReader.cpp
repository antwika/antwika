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

    Bitmap PngReader::read(std::istream &inputStream) const
    {
        const std::vector<unsigned char> bytes{
            std::istreambuf_iterator<char>(inputStream),
            std::istreambuf_iterator<char>()};

        const auto decodedImage = detail::decodeImage(
            bytes.data(),
            static_cast<int>(bytes.size()),
            static_cast<int>(kBytesPerPixel));

        const OwnedPixels pixels{decodedImage.pixels};

        if (pixels == nullptr)
        {
            throw GfxError(
                std::string("gfx: could not decode a PNG: ")
                + detail::decodeFailureReason());
        }

        const auto count = static_cast<std::size_t>(decodedImage.width)
            * static_cast<std::size_t>(decodedImage.height) * kBytesPerPixel;

        return Bitmap{
            .size = {
                .width = static_cast<std::uint32_t>(decodedImage.width),
                .height = static_cast<std::uint32_t>(decodedImage.height)},
            .pixels = std::vector<std::uint8_t>(
                pixels.get(), pixels.get() + count)};
    }

}
