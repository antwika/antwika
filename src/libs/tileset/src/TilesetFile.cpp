#include "antwika/tileset/TilesetFile.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>
#include <vector>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/GfxError.hpp>
#include <antwika/gfx/PngReader.hpp>
#include <antwika/gfx/PngWriter.hpp>

#include "antwika/tileset/Atlas.hpp"
#include "antwika/tileset/PixelClass.hpp"
#include "antwika/tileset/Sprite.hpp"
#include "antwika/tileset/TilesetError.hpp"
#include "antwika/tileset/TilesetJson.hpp"

namespace antwika::tileset
{

    namespace
    {
        constexpr gfx::Color kStorageInk{
            .red = 255, .green = 255, .blue = 255, .alpha = 255};

        constexpr gfx::Color kStoragePaper{
            .red = 128, .green = 128, .blue = 128, .alpha = 255};

        constexpr std::uint32_t kInkLuminance = 192;

        [[nodiscard]] std::uint32_t luminanceAt(
            const gfx::Bitmap &bitmap, const std::size_t offset)
        {
            return (54U * bitmap.pixels[offset]
                    + 183U * bitmap.pixels[offset + 1]
                    + 19U * bitmap.pixels[offset + 2])
                   / 256U;
        }

        [[nodiscard]] PixelClass classAt(
            const gfx::Bitmap &bitmap, const std::size_t offset)
        {
            if (bitmap.pixels[offset + 3] == 0)
            {
                return PixelClass::Blank;
            }

            return luminanceAt(bitmap, offset) >= kInkLuminance
                       ? PixelClass::Ink
                       : PixelClass::Paper;
        }

        [[nodiscard]] TilesetError fileError(
            const std::string &what,
            const std::filesystem::path &path)
        {
            return TilesetError(what + ": " + path.string());
        }

        [[nodiscard]] std::filesystem::path layerPath(
            const std::filesystem::path &directory,
            const std::size_t index)
        {
            return directory
                / ("layer-" + std::to_string(index) + ".png");
        }

        void readFrame(
            SpriteFrame &frame,
            const gfx::Bitmap &bitmap,
            const std::size_t row,
            const std::uint8_t slot)
        {
            for (std::int32_t y = 0; y < kSpriteSide; ++y)
            {
                for (std::int32_t x = 0; x < kSpriteSide; ++x)
                {
                    const auto offset =
                        ((row * static_cast<std::size_t>(kSpriteSide)
                          + static_cast<std::size_t>(y))
                             * static_cast<std::size_t>(kAtlasWidth)
                         + static_cast<std::size_t>(
                             slot * kSpriteSide + x))
                        * gfx::kBytesPerPixel;

                    frame.pixels
                        [static_cast<std::size_t>(
                            y * kSpriteSide + x)] =
                        classAt(bitmap, offset);
                }
            }
        }

        void writeLayerImage(
            const std::filesystem::path &path, const Layer &layer)
        {
            std::ofstream out(path, std::ios::binary);

            if (!out)
            {
                throw fileError(
                    "the layer image cannot be opened for writing",
                    path);
            }

            try
            {
                gfx::PngWriter{}.write(layerBitmapOf(layer), out);
            }
            catch (const gfx::GfxError &error) // GCOVR_EXCL_LINE
            {
                throw TilesetError(
                    std::string("the layer image cannot be written: ")
                    + error.what());
            }
        }

        [[nodiscard]] gfx::Bitmap readLayerImage(
            const std::filesystem::path &path)
        {
            std::ifstream in(path, std::ios::binary);

            if (!in)
            {
                throw fileError(
                    "the layer image cannot be opened for reading",
                    path);
            }

            try
            {
                return gfx::PngReader{}.read(in);
            }
            catch (const std::exception &error) // GCOVR_EXCL_LINE
            {
                throw TilesetError(
                    std::string(
                        "the layer image cannot be decoded: ")
                    + error.what());
            }
        } // GCOVR_EXCL_LINE
    }

    gfx::Bitmap layerBitmapOf(const Layer &layer)
    {
        Tileset scratch;
        scratch.layers.clear();
        scratch.layers.push_back(layer);

        return bakeAtlas(scratch, kStorageInk, kStoragePaper);
    }

    void readLayerBitmap(Layer &layer, const gfx::Bitmap &bitmap)
    {
        const gfx::Size expected{
            .width = static_cast<std::uint32_t>(kAtlasWidth),
            .height = static_cast<std::uint32_t>(
                layer.sprites.size()
                * static_cast<std::size_t>(kSpriteSide))};

        if (bitmap.size != expected || !bitmap.isComplete())
        {
            throw TilesetError(
                "the layer image is not "
                + std::to_string(expected.width) + " by "
                + std::to_string(expected.height));
        }

        for (std::size_t row = 0; row < layer.sprites.size(); ++row)
        {
            auto &sprite = layer.sprites[row];
            const auto frames =
                std::min(sprite.frameCount, kMaxFrames);

            for (std::uint8_t slot = 0; slot < frames; ++slot)
            {
                readFrame(sprite.frames[slot], bitmap, row, slot);
            }
        }
    }

    void saveTileset(
        const std::filesystem::path &directory, const Tileset &set)
    {
        std::error_code failure;
        std::filesystem::create_directories(directory, failure);

        if (failure)
        {
            throw fileError(
                "the tileset directory cannot be created", directory);
        }

        const auto documentPath = directory / "tileset.json";
        std::ofstream out(documentPath);

        if (!out)
        {
            throw fileError(
                "the tileset file cannot be opened for writing",
                documentPath);
        }

        out << toJson(set) << '\n';
        out.flush();

        if (!out)
        {
            throw fileError(
                "the tileset file cannot be written", documentPath);
        }

        for (std::size_t index = 0;
             index < set.layers.size();
             ++index)
        {
            const auto path = layerPath(directory, index);

            if (set.layers[index].sprites.empty())
            {
                std::error_code ignored;
                std::filesystem::remove(path, ignored);
                continue;
            }

            writeLayerImage(path, set.layers[index]);
        }
    }

    Tileset loadTileset(const std::filesystem::path &directory)
    {
        const auto documentPath = directory / "tileset.json";
        std::ifstream in(documentPath);

        if (!in)
        {
            throw fileError(
                "the tileset file cannot be opened for reading",
                documentPath);
        }

        const std::string text{
            std::istreambuf_iterator<char>(in),
            std::istreambuf_iterator<char>()};

        auto set = tilesetFromJson(text);

        for (std::size_t index = 0;
             index < set.layers.size();
             ++index)
        {
            if (set.layers[index].sprites.empty())
            {
                continue;
            }

            readLayerBitmap(
                set.layers[index],
                readLayerImage(layerPath(directory, index)));
        }

        return set;
    }

    std::vector<std::string> listTilesets(
        const std::filesystem::path &assetsDir)
    {
        std::vector<std::string> names;
        std::error_code missing;

        for (const auto &entry : std::filesystem::directory_iterator(
                 assetsDir, missing))
        {
            const auto document = entry.path() / "tileset.json";

            if (entry.is_directory()
                && std::filesystem::is_regular_file(document))
            {
                names.push_back(entry.path().filename().string());
            }
        }

        std::ranges::sort(names);

        return names;
    } // GCOVR_EXCL_LINE

    std::vector<Tileset> loadTilesetLibrary(
        const std::filesystem::path &assetsDir)
    {
        std::vector<Tileset> sets;

        for (const auto &name : listTilesets(assetsDir))
        {
            try
            {
                sets.push_back(loadTileset(assetsDir / name));
            }
            catch (const TilesetError &) // GCOVR_EXCL_LINE
            {
                continue;
            }
        }

        std::ranges::sort(
            sets,
            [](const Tileset &left, const Tileset &right)
            { return left.name < right.name; });

        return sets;
    } // GCOVR_EXCL_LINE

}
