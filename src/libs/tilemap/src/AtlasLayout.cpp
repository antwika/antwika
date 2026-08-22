#include "antwika/tilemap/AtlasLayout.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/SizeF.hpp>

namespace antwika::tilemap
{

    namespace
    {
        constexpr std::size_t kTiles =
            static_cast<std::size_t>(kAtlasColumns * kAtlasRows);
    }

    gfx::Size atlasSize(const gfx::Size tileSize)
    {
        const auto columns =
            static_cast<std::uint32_t>(kAtlasColumns);
        const auto rows = static_cast<std::uint32_t>(kAtlasRows);
        const auto padding =
            static_cast<std::uint32_t>(kAtlasPadding);

        return gfx::Size{
            .width = columns * tileSize.width + (columns - 1) * padding,
            .height = rows * tileSize.height + (rows - 1) * padding};
    }

    gfx::Bitmap blankAtlas(const gfx::Size tileSize)
    {
        const auto wholeSize = atlasSize(tileSize);

        return gfx::Bitmap{
            .size = wholeSize,
            .pixels = std::vector<std::uint8_t>(
                static_cast<std::size_t>(wholeSize.width)
                    * wholeSize.height * gfx::kBytesPerPixel,
                0)};
    } // GCOVR_EXCL_LINE

    gfx::RectF tilePixels(
        const std::size_t index, const gfx::Size tileSize)
    {
        const auto tileIndex = index % kTiles;
        const auto column = static_cast<std::uint32_t>(
            tileIndex % static_cast<std::size_t>(kAtlasColumns));
        const auto row = static_cast<std::uint32_t>(
            tileIndex / static_cast<std::size_t>(kAtlasColumns));
        const auto padding =
            static_cast<std::uint32_t>(kAtlasPadding);

        return gfx::RectF(
            gfx::PointF{
                static_cast<float>(column * (tileSize.width + padding)),
                static_cast<float>(row * (tileSize.height + padding))},
            gfx::SizeF{
                static_cast<float>(tileSize.width),
                static_cast<float>(tileSize.height)});
    }

    gfx::RectF tileCoords(
        const std::size_t index, const gfx::Size tileSize)
    {
        const auto wholeSize = atlasSize(tileSize);
        const auto tileRect = tilePixels(index, tileSize);
        const auto width = static_cast<float>(wholeSize.width);
        const auto height = static_cast<float>(wholeSize.height);

        return gfx::RectF(
            gfx::PointF{
                tileRect.originPoint.x / width,
                tileRect.originPoint.y / height},
            gfx::SizeF{
                tileRect.size.width / width,
                tileRect.size.height / height});
    }

}
