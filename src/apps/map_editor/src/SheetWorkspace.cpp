#include "antwika/map_editor/SheetWorkspace.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>

#include <antwika/autotile/SheetLayout.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/GfxError.hpp>
#include <antwika/gfx/Glyphs.hpp>
#include <antwika/gfx/PngReader.hpp>
#include <antwika/gfx/PngWriter.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/enums/Enumeration.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/ui/Theme.hpp>

#include "antwika/map_editor/PlaceholderSheets.hpp"

namespace antwika::map_editor
{

    namespace
    {
        using antwika::autotile::kSheetHeight;
        using antwika::autotile::kSheetWidth;
        using antwika::gfx::Bitmap;
        using antwika::gfx::Color;
        using antwika::gfx::Point;
        using antwika::gfx::RectF;

        constexpr Color kInk{
            .red = 255, .green = 255, .blue = 255, .alpha = 255};

        constexpr Color kBackdropLight{
            .red = 74, .green = 76, .blue = 84};

        constexpr Color kBackdropDark{
            .red = 58, .green = 60, .blue = 66};

        constexpr Color kPixelGridColor{
            .red = 110, .green = 114, .blue = 124, .alpha = 70};

        constexpr Color kSlotGuideColor{
            .red = 140, .green = 146, .blue = 160, .alpha = 150};

        constexpr Color kBandGuideColor{
            .red = 214, .green = 190, .blue = 110, .alpha = 200};

        constexpr Color kHoverColor{
            .red = 244, .green = 208, .blue = 63, .alpha = 220};

        constexpr Color kLabelColor{
            .red = 214, .green = 224, .blue = 216};

        constexpr std::int32_t kWorkspaceLeft = 80;

        constexpr std::int32_t kWorkspaceTop = 20;

        constexpr std::int32_t kWorkspaceWidth =
            static_cast<std::int32_t>(kSheetWidth) * kSheetZoom;

        constexpr std::int32_t kWorkspaceHeight =
            static_cast<std::int32_t>(kSheetHeight) * kSheetZoom;

        constexpr std::int32_t kBackdropStep = 2;

        [[nodiscard]] std::size_t pixelOffset(
            const Bitmap &sheet, const Point pixel)
        {
            return (static_cast<std::size_t>(pixel.y)
                        * sheet.size.width
                    + static_cast<std::size_t>(pixel.x))
                   * gfx::kBytesPerPixel;
        }

        [[nodiscard]] bool inSheet(
            const Bitmap &sheet, const Point pixel) noexcept
        {
            return pixel.x >= 0 && pixel.y >= 0
                   && pixel.x
                          < static_cast<std::int32_t>(sheet.size.width)
                   && pixel.y < static_cast<std::int32_t>(
                          sheet.size.height);
        }

        void recolorOpaqueToWhite(Bitmap &sheet)
        {
            for (std::size_t offset = 0;
                 offset + 3 < sheet.pixels.size();
                 offset += gfx::kBytesPerPixel)
            {
                if (sheet.pixels[offset + 3] == 0)
                {
                    continue;
                }

                sheet.pixels[offset] = 255;
                sheet.pixels[offset + 1] = 255;
                sheet.pixels[offset + 2] = 255;
            }
        }
    }

    std::optional<Point> sheetPixelAt(const Point canvas) noexcept
    {
        const auto localX = canvas.x - kWorkspaceLeft;
        const auto localY = canvas.y - kWorkspaceTop;

        if (localX < 0 || localY < 0 || localX >= kWorkspaceWidth
            || localY >= kWorkspaceHeight)
        {
            return std::nullopt;
        }

        return Point{
            .x = localX / kSheetZoom, .y = localY / kSheetZoom};
    }

    std::string slotLabelAt(const Point pixel)
    {
        if (pixel.x < 0 || pixel.y < 0
            || pixel.x >= static_cast<std::int32_t>(kSheetWidth)
            || pixel.y >= static_cast<std::int32_t>(kSheetHeight))
        {
            return {};
        }

        const auto column = pixel.x / 8;

        if (pixel.y < 32)
        {
            const auto mask = (pixel.y / 8) * 4 + column;

            return "mask " + std::to_string(mask);
        }

        if (pixel.y < 40)
        {
            constexpr std::array<std::string_view, 4> kSpecial{
                "wall band", "wall rim", "bridge deck", "shade"};

            return std::string(
                kSpecial[static_cast<std::size_t>(column)]);
        }

        constexpr std::array<std::string_view, 4> kVariants{
            "variant 1", "variant 2", "water frame B", "spare"};

        return std::string(
            kVariants[static_cast<std::size_t>(column)]);
    }

    bool setSheetPixel(
        Bitmap &sheet, const Point pixel, const bool ink)
    {
        if (!sheet.isComplete() || !inSheet(sheet, pixel))
        {
            return false;
        }

        const auto offset = pixelOffset(sheet, pixel);
        const Color color = ink ? kInk : Color{};

        if (sheet.pixels[offset] == color.red
            && sheet.pixels[offset + 1] == color.green
            && sheet.pixels[offset + 2] == color.blue
            && sheet.pixels[offset + 3] == color.alpha)
        {
            return false;
        }

        sheet.pixels[offset] = color.red;
        sheet.pixels[offset + 1] = color.green;
        sheet.pixels[offset + 2] = color.blue;
        sheet.pixels[offset + 3] = color.alpha;

        return true;
    }

    bool sheetPixelInked(const Bitmap &sheet, const Point pixel)
    {
        if (!sheet.isComplete() || !inSheet(sheet, pixel))
        {
            return false;
        }

        return sheet.pixels[pixelOffset(sheet, pixel) + 3] != 0;
    }

    gfx::Bitmap loadSheetOrPlaceholder(
        const std::filesystem::path &directory,
        const tilemap::TerrainClass terrain,
        log::ILogger &logger)
    {
        const auto path = sheetPathFor(directory, terrain);

        if (std::filesystem::is_regular_file(path))
        {
            try
            {
                std::ifstream in(path, std::ios::binary);
                auto bitmap = gfx::PngReader{}.read(in);

                if (bitmap.size.width == kSheetWidth
                    && bitmap.size.height == kSheetHeight)
                {
                    recolorOpaqueToWhite(bitmap);
                    return bitmap;
                }

                logger.log(
                    log::Level::Warning,
                    "map_editor: wrong sheet size in "
                        + path.string());
            }
            catch (const gfx::GfxError &error)
            {
                logger.log(log::Level::Warning, error.what());
            }
        }

        return placeholderSheet(terrain, kInk);
    }

    std::filesystem::path sheetPathFor(
        const std::filesystem::path &directory,
        const tilemap::TerrainClass terrain)
    {
        return directory
               / (std::string(tilemap::toString(terrain)) + ".png");
    }

    std::optional<std::string> saveSheet(
        const Bitmap &sheet,
        const std::filesystem::path &directory,
        const tilemap::TerrainClass terrain)
    {
        std::error_code made;
        std::filesystem::create_directories(directory, made);

        const auto path = sheetPathFor(directory, terrain);
        std::ofstream out(path, std::ios::binary);

        if (!out)
        {
            return "cannot open " + path.string();
        }

        gfx::PngWriter{}.write(sheet, out);

        if (!out.good())
        {
            return "cannot write " + path.string();
        }

        return std::nullopt;
    }

    namespace
    {
        void applyStrokePixel(
            SheetDoc &doc, const Point pixel, const bool ink)
        {
            if (setSheetPixel(doc.image, pixel, ink))
            {
                doc.dirty = true;
                ++doc.revision;
            }
        }
    }

    void applySheetGesture(
        EditorStore &store, const SheetGesture &gesture)
    {
        auto *active = activeSheet(store);

        if (active == nullptr)
        {
            return;
        }

        auto &doc = *active;
        auto &tiles = store.tiles;

        if (gesture.kind == GestureKind::Press)
        {
            doc.undoStack.push_back(doc.image);
            doc.redoStack.clear();
            tiles.stroke = true;
            tiles.strokeInk = gesture.ink;
            applyStrokePixel(doc, gesture.pixel, gesture.ink);
            return;
        }

        if (gesture.kind == GestureKind::Move)
        {
            if (tiles.stroke)
            {
                applyStrokePixel(doc, gesture.pixel, tiles.strokeInk);
            }

            return;
        }

        if (!tiles.stroke)
        {
            return;
        }

        tiles.stroke = false;

        if (!doc.undoStack.empty()
            && doc.undoStack.back() == doc.image)
        {
            doc.undoStack.pop_back();
        }
    }

    void sheetUndo(EditorStore &store)
    {
        auto *active = activeSheet(store);

        if (active == nullptr || active->undoStack.empty())
        {
            return;
        }

        auto &doc = *active;

        doc.redoStack.push_back(doc.image);
        doc.image = doc.undoStack.back();
        doc.undoStack.pop_back();
        doc.dirty = true;
        ++doc.revision;
    }

    void sheetRedo(EditorStore &store)
    {
        auto *active = activeSheet(store);

        if (active == nullptr || active->redoStack.empty())
        {
            return;
        }

        auto &doc = *active;

        doc.undoStack.push_back(doc.image);
        doc.image = doc.redoStack.back();
        doc.redoStack.pop_back();
        doc.dirty = true;
        ++doc.revision;
    }

    void saveActiveTerrainSheet(
        EditorStore &store, log::ILogger &logger)
    {
        auto &doc =
            store.tiles.docs[enums::index(store.state.brush)];
        const auto error = saveSheet(
            doc.image, store.tiles.directory, store.state.brush);

        if (error.has_value())
        {
            logger.log(log::Level::Error, *error);
            return;
        }

        doc.dirty = false;
        logger.log(
            log::Level::Info,
            "map_editor: saved "
                + sheetPathFor(store.tiles.directory, store.state.brush)
                      .string());
    }

    void drawPixelOutline(
        gfx::ViewportRenderer &view,
        const gfx::PointF origin,
        const float zoom)
    {
        const auto color = antwika::ui::Theme{}.focusRing;

        view.drawRect(
            RectF(origin, {zoom, 1.0F}), color);
        view.drawRect(
            RectF(
                {origin.x, origin.y + zoom - 1.0F}, {zoom, 1.0F}),
            color);
        view.drawRect(
            RectF(
                {origin.x, origin.y + 1.0F}, {1.0F, zoom - 2.0F}),
            color);
        view.drawRect(
            RectF(
                {origin.x + zoom - 1.0F, origin.y + 1.0F},
                {1.0F, zoom - 2.0F}),
            color);
    }

    void drawSheetWorkspace(
        gfx::ViewportRenderer &view,
        const gfx::ITexture &sheet,
        const Bitmap &image,
        const std::optional<Point> hover)
    {
        const auto left = static_cast<float>(kWorkspaceLeft);
        const auto top = static_cast<float>(kWorkspaceTop);
        const auto zoom = static_cast<float>(kSheetZoom);

        for (std::int32_t y = 0;
             y < static_cast<std::int32_t>(kSheetHeight);
             y += kBackdropStep)
        {
            for (std::int32_t x = 0;
                 x < static_cast<std::int32_t>(kSheetWidth);
                 x += kBackdropStep)
            {
                const bool light =
                    ((x / kBackdropStep) + (y / kBackdropStep)) % 2
                    == 0;

                view.drawRect(
                    RectF(
                        {left + static_cast<float>(x) * zoom,
                         top + static_cast<float>(y) * zoom},
                        {zoom * kBackdropStep, zoom * kBackdropStep}),
                    light ? kBackdropLight : kBackdropDark);
            }
        }

        view.drawTexture(
            sheet,
            RectF(
                {0.0F, 0.0F},
                {static_cast<float>(kSheetWidth),
                 static_cast<float>(kSheetHeight)}),
            RectF(
                {left, top},
                {static_cast<float>(kWorkspaceWidth),
                 static_cast<float>(kWorkspaceHeight)}),
            gfx::Color{.red = 255, .green = 255, .blue = 255});

        if (kSheetZoom >= kPixelGridMinZoom)
        {
            for (std::int32_t x = 0;
                 x <= static_cast<std::int32_t>(kSheetWidth);
                 ++x)
            {
                const auto lineX =
                    left + static_cast<float>(x) * zoom;

                view.drawLine(
                    {lineX, top},
                    {lineX,
                     top + static_cast<float>(kWorkspaceHeight)},
                    kPixelGridColor);
            }

            for (std::int32_t y = 0;
                 y <= static_cast<std::int32_t>(kSheetHeight);
                 ++y)
            {
                const auto lineY =
                    top + static_cast<float>(y) * zoom;

                view.drawLine(
                    {left, lineY},
                    {left + static_cast<float>(kWorkspaceWidth),
                     lineY},
                    kPixelGridColor);
            }
        }

        for (std::int32_t x = 0;
             x <= static_cast<std::int32_t>(kSheetWidth);
             x += 8)
        {
            const auto lineX = left + static_cast<float>(x) * zoom;

            view.drawLine(
                {lineX, top},
                {lineX, top + static_cast<float>(kWorkspaceHeight)},
                kSlotGuideColor);
        }

        for (std::int32_t y = 0;
             y <= static_cast<std::int32_t>(kSheetHeight);
             y += 8)
        {
            const auto lineY = top + static_cast<float>(y) * zoom;
            const bool band = y == 32 || y == 40 || y == 48;

            view.drawLine(
                {left, lineY},
                {left + static_cast<float>(kWorkspaceWidth), lineY},
                band ? kBandGuideColor : kSlotGuideColor);
        }

        if (!hover.has_value())
        {
            return;
        }

        view.drawRect(
            RectF(
                {left + static_cast<float>(hover->x) * zoom,
                 top + static_cast<float>(hover->y) * zoom},
                {zoom, zoom}),
            Color{
                .red = kHoverColor.red,
                .green = kHoverColor.green,
                .blue = kHoverColor.blue,
                .alpha = 90});
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
            slotLabelAt(*hover) + " " + std::to_string(hover->x)
                + "," + std::to_string(hover->y),
            gfx::encodeTextScale(gfx::TextFace::Small, 1),
            kLabelColor);
    }

}
