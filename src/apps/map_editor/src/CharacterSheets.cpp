#include "antwika/map_editor/CharacterSheets.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <fstream>
#include <string_view>
#include <system_error>
#include <utility>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/GfxError.hpp>
#include <antwika/gfx/Glyphs.hpp>
#include <antwika/gfx/PngReader.hpp>
#include <antwika/gfx/PngWriter.hpp>
#include <antwika/log/Level.hpp>

#include "antwika/map_editor/SheetWorkspace.hpp"

namespace antwika::map_editor
{

    namespace
    {
        using antwika::gfx::Bitmap;
        using antwika::gfx::Color;
        using antwika::gfx::Point;
        using antwika::gfx::RectF;

        constexpr Color kInk{
            .red = 214, .green = 224, .blue = 216, .alpha = 255};

        constexpr Color kBackdropLight{
            .red = 74, .green = 76, .blue = 84};

        constexpr Color kBackdropDark{
            .red = 58, .green = 60, .blue = 66};

        constexpr Color kPixelGridColor{
            .red = 110, .green = 114, .blue = 124, .alpha = 70};

        constexpr Color kFrameGuideColor{
            .red = 140, .green = 146, .blue = 160, .alpha = 150};

        constexpr Color kHoverColor{
            .red = 244, .green = 208, .blue = 63, .alpha = 90};

        constexpr Color kLabelColor{
            .red = 214, .green = 224, .blue = 216};

        constexpr Color kWhite{
            .red = 255, .green = 255, .blue = 255};

        constexpr std::int32_t kLeft = 32;

        constexpr std::int32_t kTop = 12;

        constexpr std::int32_t kExtent =
            static_cast<std::int32_t>(kCharacterSize) * kCharacterZoom;

        constexpr std::array<std::string_view, 4> kRowNames{
            "walk_down", "walk_up", "walk_left", "walk_right"};

        constexpr std::string_view kSidecar =
            "{\n"
            "  \"size\": 64,\n"
            "  \"frame\": 16,\n"
            "  \"columns\": 4,\n"
            "  \"idle_column\": 0,\n"
            "  \"rows\": [\"walk_down\", \"walk_up\","
            " \"walk_left\", \"walk_right\"]\n"
            "}\n";

        [[nodiscard]] std::filesystem::path pngPathFor(
            const std::filesystem::path &directory,
            const std::string &name)
        {
            return directory / (name + ".png");
        }

        [[nodiscard]] std::filesystem::path sidecarPathFor(
            const std::filesystem::path &directory,
            const std::string &name)
        {
            return directory / (name + ".json");
        }

        void putPixel(
            Bitmap &sheet, const std::int32_t x, const std::int32_t y)
        {
            const auto offset =
                (static_cast<std::size_t>(y) * kCharacterSize
                 + static_cast<std::size_t>(x))
                * gfx::kBytesPerPixel;

            sheet.pixels[offset] = kInk.red;
            sheet.pixels[offset + 1] = kInk.green;
            sheet.pixels[offset + 2] = kInk.blue;
            sheet.pixels[offset + 3] = kInk.alpha;
        }

        void drawFigure(
            Bitmap &sheet,
            const std::int32_t row,
            const std::int32_t frame)
        {
            const auto originX = frame * 16;
            const auto originY = row * 16;

            const auto put =
                [&](const std::int32_t x, const std::int32_t y)
            {
                putPixel(sheet, originX + x, originY + y);
            };

            for (std::int32_t y = 2; y <= 6; ++y)
            {
                for (std::int32_t x = 6; x <= 10; ++x)
                {
                    put(x, y);
                }
            }

            for (std::int32_t y = 7; y <= 11; ++y)
            {
                for (std::int32_t x = 5; x <= 11; ++x)
                {
                    put(x, y);
                }
            }

            const bool spread = frame % 2 == 1;
            const auto leftLeg = spread ? 5 : 6;
            const auto rightLeg = spread ? 11 : 10;

            for (std::int32_t y = 12; y <= 15; ++y)
            {
                put(leftLeg, y);
                put(leftLeg + 1, y);
                put(rightLeg - 1, y);
                put(rightLeg, y);
            }
        }

        void markFace(
            Bitmap &sheet,
            const std::int32_t row,
            const std::int32_t frame)
        {
            const auto originX = frame * 16;
            const auto originY = row * 16;

            const auto clear =
                [&](const std::int32_t x, const std::int32_t y)
            {
                const auto offset =
                    (static_cast<std::size_t>(originY + y)
                         * kCharacterSize
                     + static_cast<std::size_t>(originX + x))
                    * gfx::kBytesPerPixel;

                sheet.pixels[offset + 3] = 0;
            };

            if (row == 0)
            {
                clear(7, 4);
                clear(9, 4);
            }
            else if (row == 2)
            {
                clear(6, 4);
            }
            else if (row == 3)
            {
                clear(10, 4);
            }
        }
    }

    std::string_view rowNameOf(const std::int32_t row)
    {
        if (row < 0 || row >= 4)
        {
            return "?";
        }

        return kRowNames[static_cast<std::size_t>(row)];
    }

    std::optional<Point> characterPixelAt(const Point canvas) noexcept
    {
        const auto localX = canvas.x - kLeft;
        const auto localY = canvas.y - kTop;

        if (localX < 0 || localY < 0 || localX >= kExtent
            || localY >= kExtent)
        {
            return std::nullopt;
        }

        return Point{
            .x = localX / kCharacterZoom,
            .y = localY / kCharacterZoom};
    }

    Bitmap placeholderCharacter()
    {
        Bitmap sheet{
            .size = {.width = kCharacterSize, .height = kCharacterSize},
            .pixels = {}};

        sheet.pixels.assign(
            static_cast<std::size_t>(kCharacterSize) * kCharacterSize
                * gfx::kBytesPerPixel,
            0);

        for (std::int32_t row = 0; row < 4; ++row)
        {
            for (std::int32_t frame = 0; frame < 4; ++frame)
            {
                drawFigure(sheet, row, frame);
                markFace(sheet, row, frame);
            }
        }

        return sheet;
    }

    std::vector<CharacterDoc> loadCharacters(
        const std::filesystem::path &directory, log::ILogger &logger)
    {
        std::vector<CharacterDoc> list;

        std::error_code missing;
        for (const auto &entry :
             std::filesystem::directory_iterator(directory, missing))
        {
            if (!entry.is_regular_file()
                || entry.path().extension() != ".png")
            {
                continue;
            }

            try
            {
                std::ifstream in(entry.path(), std::ios::binary);
                auto bitmap = gfx::PngReader{}.read(in);

                if (bitmap.size.width != kCharacterSize
                    || bitmap.size.height != kCharacterSize)
                {
                    logger.log(
                        log::Level::Warning,
                        "map_editor: wrong character size in "
                            + entry.path().string());
                    continue;
                }

                CharacterDoc character{
                    .name = entry.path().stem().string()};

                character.sheet.image = std::move(bitmap);
                list.push_back(std::move(character));
            }
            catch (const gfx::GfxError &error)
            {
                logger.log(log::Level::Warning, error.what());
            }
        }

        std::ranges::sort(
            list,
            [](const CharacterDoc &a, const CharacterDoc &b)
            { return a.name < b.name; });

        return list;
    } // GCOVR_EXCL_LINE

    std::optional<std::string> saveCharacter(
        const CharacterDoc &character,
        const std::filesystem::path &directory)
    {
        std::error_code made;
        std::filesystem::create_directories(directory, made);

        const auto png = pngPathFor(directory, character.name);

        {
            std::ofstream out(png, std::ios::binary);

            if (!out)
            {
                return "cannot open " + png.string();
            }

            gfx::PngWriter{}.write(character.sheet.image, out);

            if (!out.good())
            {
                return "cannot write " + png.string();
            }
        }

        std::ofstream sidecar(
            sidecarPathFor(directory, character.name));

        if (!sidecar)
        {
            return "cannot write the sidecar for " + character.name;
        }

        sidecar << kSidecar;

        return std::nullopt;
    }

    void deleteCharacterFiles(
        const std::string &name,
        const std::filesystem::path &directory)
    {
        std::error_code ignored;

        std::filesystem::remove(pngPathFor(directory, name), ignored);
        std::filesystem::remove(
            sidecarPathFor(directory, name), ignored);
    }

    void saveSelectedCharacter(
        EditorStore &store, log::ILogger &logger)
    {
        auto &characters = store.characters;

        if (characters.selected >= characters.list.size())
        {
            characters.message = "nothing to save";
            return;
        }

        auto &character = characters.list[characters.selected];
        const auto error =
            saveCharacter(character, characters.directory);

        if (error.has_value())
        {
            characters.message = *error;
            logger.log(log::Level::Error, *error);
            return;
        }

        character.sheet.dirty = false;
        characters.message.clear();
        logger.log(
            log::Level::Info,
            "map_editor: saved character " + character.name);
    }

    RectF characterFrameSource(
        const std::int32_t row, const std::int32_t frame) noexcept
    {
        return RectF(
            {static_cast<float>(frame)
                 * static_cast<float>(kFrameSize),
             static_cast<float>(row) * static_cast<float>(kFrameSize)},
            {static_cast<float>(kFrameSize),
             static_cast<float>(kFrameSize)});
    }

    void drawCharacterWorkspace(
        gfx::ViewportRenderer &view,
        const gfx::ITexture &sheet,
        const Bitmap &image,
        const std::optional<Point> hover,
        const std::uint32_t tick)
    {
        const auto left = static_cast<float>(kLeft);
        const auto top = static_cast<float>(kTop);
        const auto zoom = static_cast<float>(kCharacterZoom);
        const auto size = static_cast<std::int32_t>(kCharacterSize);

        for (std::int32_t y = 0; y < size; y += 4)
        {
            for (std::int32_t x = 0; x < size; x += 4)
            {
                const bool light = ((x / 4) + (y / 4)) % 2 == 0;

                view.drawRect(
                    RectF(
                        {left + static_cast<float>(x) * zoom,
                         top + static_cast<float>(y) * zoom},
                        {zoom * 4, zoom * 4}),
                    light ? kBackdropLight : kBackdropDark);
            }
        }

        view.drawTexture(
            sheet,
            RectF(
                {0.0F, 0.0F},
                {static_cast<float>(kCharacterSize),
                 static_cast<float>(kCharacterSize)}),
            RectF(
                {left, top},
                {static_cast<float>(kExtent),
                 static_cast<float>(kExtent)}),
            kWhite);

        if (kCharacterZoom >= kPixelGridMinZoom)
        {
            for (std::int32_t x = 0; x <= size; ++x)
            {
                const auto lineX =
                    left + static_cast<float>(x) * zoom;

                view.drawLine(
                    {lineX, top},
                    {lineX, top + static_cast<float>(kExtent)},
                    kPixelGridColor);
            }

            for (std::int32_t y = 0; y <= size; ++y)
            {
                const auto lineY =
                    top + static_cast<float>(y) * zoom;

                view.drawLine(
                    {left, lineY},
                    {left + static_cast<float>(kExtent), lineY},
                    kPixelGridColor);
            }
        }

        for (std::int32_t x = 0; x <= size;
             x += static_cast<std::int32_t>(kFrameSize))
        {
            const auto lineX = left + static_cast<float>(x) * zoom;

            view.drawLine(
                {lineX, top},
                {lineX, top + static_cast<float>(kExtent)},
                kFrameGuideColor);
        }

        for (std::int32_t y = 0; y <= size;
             y += static_cast<std::int32_t>(kFrameSize))
        {
            const auto lineY = top + static_cast<float>(y) * zoom;

            view.drawLine(
                {left, lineY},
                {left + static_cast<float>(kExtent), lineY},
                kFrameGuideColor);
        }

        const auto row =
            hover.has_value()
                ? hover->y / static_cast<std::int32_t>(kFrameSize)
                : 0;
        const auto frame =
            static_cast<std::int32_t>((tick / 8) % 4);

        view.drawRect(
            RectF({0.0F, 20.0F}, {32.0F, 32.0F}), kBackdropDark);
        view.drawTexture(
            sheet,
            characterFrameSource(row, frame),
            RectF({8.0F, 28.0F}, {16.0F, 16.0F}),
            kWhite);

        if (!hover.has_value())
        {
            return;
        }

        view.drawRect(
            RectF(
                {left + static_cast<float>(hover->x) * zoom,
                 top + static_cast<float>(hover->y) * zoom},
                {zoom, zoom}),
            kHoverColor);
        drawPixelOutline(
            view,
            {left + static_cast<float>(hover->x) * zoom,
             top + static_cast<float>(hover->y) * zoom},
            zoom);

        const bool inked = sheetPixelInked(image, *hover);

        view.drawRect(
            RectF({4.0F, 262.0F}, {6.0F, 6.0F}),
            inked ? kInk : kBackdropDark);

        view.drawText(
            {14.0F, 262.0F},
            std::string(rowNameOf(row)) + " f"
                + std::to_string(
                    hover->x / static_cast<std::int32_t>(kFrameSize))
                + " " + std::to_string(hover->x) + ","
                + std::to_string(hover->y),
            gfx::encodeTextScale(gfx::TextFace::Small, 1),
            kLabelColor);
    }

}
