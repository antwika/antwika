#include "antwika/map_editor/SheetWorkspace.hpp"

#include <nlohmann/json.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#include <antwika/autotile/SheetLayout.hpp>
#include <antwika/enums/Enumeration.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/GfxError.hpp>
#include <antwika/gfx/Glyphs.hpp>
#include <antwika/gfx/PngReader.hpp>
#include <antwika/gfx/PngWriter.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/RectF.hpp>
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
        using antwika::gfx::PointF;
        using antwika::gfx::RectF;

        constexpr Color kInk{
            .red = 255, .green = 255, .blue = 255, .alpha = 255};

        constexpr Color kPaper{
            .red = 128, .green = 128, .blue = 128, .alpha = 255};

        constexpr std::uint32_t kInkLuminance = 192;

        [[nodiscard]] std::uint32_t luminanceAt(
            const Bitmap &sheet, const std::size_t offset)
        {
            return (54U * sheet.pixels[offset]
                    + 183U * sheet.pixels[offset + 1]
                    + 19U * sheet.pixels[offset + 2])
                   / 256U;
        }

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

        constexpr std::int32_t kWorkspaceLeft = 16;

        constexpr std::int32_t kWorkspaceTop = 14;

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

        constexpr std::array<std::pair<char, std::uint8_t>, 4>
            kEdgeLetters{
                {{'N', autotile::kEdgeNorth},
                 {'E', autotile::kEdgeEast},
                 {'S', autotile::kEdgeSouth},
                 {'W', autotile::kEdgeWest}}};

        [[nodiscard]] std::string edgesToText(
            const std::uint8_t edges)
        {
            std::string text;

            for (const auto &[letter, bit] : kEdgeLetters)
            {
                if ((edges & bit) != 0)
                {
                    text.push_back(letter);
                }
            }

            return text;
        }

        [[nodiscard]] std::uint8_t edgesFromText(
            const std::string &text)
        {
            std::uint8_t edges = 0;

            for (const auto &[letter, bit] : kEdgeLetters)
            {
                if (text.find(letter) != std::string::npos)
                {
                    edges |= bit;
                }
            }

            return edges;
        }
    }

    void normalizeSheetClasses(Bitmap &sheet)
    {
        for (std::size_t offset = 0;
             offset + 3 < sheet.pixels.size();
             offset += gfx::kBytesPerPixel)
        {
            if (sheet.pixels[offset + 3] == 0)
            {
                continue;
            }

            const auto &color =
                luminanceAt(sheet, offset) >= kInkLuminance
                    ? kInk
                    : kPaper;

            sheet.pixels[offset] = color.red;
            sheet.pixels[offset + 1] = color.green;
            sheet.pixels[offset + 2] = color.blue;
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

        if (pixel.x < 64 && pixel.y < 64)
        {
            const auto mask =
                (pixel.y / 16) * 4 + pixel.x / 16;

            return "mask " + std::to_string(mask);
        }

        if (pixel.x >= 64 && pixel.y < 64)
        {
            const auto slot =
                (pixel.y / 16) * 2 + (pixel.x - 64) / 16;

            return slot < 7 ? "variant " + std::to_string(slot + 1)
                            : "water frame B";
        }

        if (pixel.y < 72 && pixel.x < 32)
        {
            constexpr std::array<std::string_view, 4> kSpecial{
                "wall band", "wall rim", "bridge deck", "shade"};

            return std::string(
                kSpecial[static_cast<std::size_t>(pixel.x / 8)]);
        }

        return "spare";
    }

    bool setSheetPixel(
        Bitmap &sheet, const Point pixel, const PixelClass value)
    {
        if (!sheet.isComplete() || !inSheet(sheet, pixel))
        {
            return false;
        }

        const auto offset = pixelOffset(sheet, pixel);
        const Color color =
            value == PixelClass::Ink
                ? kInk
                : (value == PixelClass::Paper ? kPaper
                                              : Color{.alpha = 0});

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

    PixelClass sheetPixelClass(
        const Bitmap &sheet, const Point pixel)
    {
        if (!sheet.isComplete() || !inSheet(sheet, pixel))
        {
            return PixelClass::Blank;
        }

        const auto offset = pixelOffset(sheet, pixel);

        if (sheet.pixels[offset + 3] == 0)
        {
            return PixelClass::Blank;
        }

        return luminanceAt(sheet, offset) >= kInkLuminance
                   ? PixelClass::Ink
                   : PixelClass::Paper;
    }

    gfx::Bitmap bakedSheet(
        const Bitmap &sheet, const Color ink, const Color paper)
    {
        auto baked = sheet;

        for (std::size_t offset = 0;
             offset + 3 < baked.pixels.size();
             offset += gfx::kBytesPerPixel)
        {
            if (baked.pixels[offset + 3] == 0)
            {
                continue;
            }

            const auto &color =
                luminanceAt(baked, offset) >= kInkLuminance
                    ? ink
                    : paper;

            baked.pixels[offset] = color.red;
            baked.pixels[offset + 1] = color.green;
            baked.pixels[offset + 2] = color.blue;
        }

        return baked;
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
                    normalizeSheetClasses(bitmap);
                    return bitmap;
                }

                const bool legacy =
                    (bitmap.size.width == 32
                     && bitmap.size.height == 48)
                    || (bitmap.size.width == 96
                        && bitmap.size.height == 64);

                if (legacy)
                {
                    logger.log(
                        log::Level::Warning,
                        "map_editor: legacy sheet layout in "
                            + path.string()
                            + ", redraw needed; using the "
                              "placeholder");
                }
                else
                {
                    logger.log(
                        log::Level::Warning,
                        "map_editor: wrong sheet size in "
                            + path.string());
                }
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

    autotile::TerrainConnectors loadConnectorsFile(
        const std::filesystem::path &directory)
    {
        autotile::TerrainConnectors connectors{};

        std::ifstream in(directory / "tiles.json");

        if (!in)
        {
            return connectors;
        }

        const auto document =
            nlohmann::json::parse(in, nullptr, false);

        if (document.is_discarded()
            || !document.contains("connectors")
            || !document.at("connectors").is_object())
        {
            return connectors;
        }

        for (const auto &[name, slots] :
             document.at("connectors").items())
        {
            for (const auto terrain :
                 enums::kAll<tilemap::TerrainClass>)
            {
                if (tilemap::toString(terrain) != name
                    || !slots.is_object())
                {
                    continue;
                }

                for (const auto &[slot, edges] : slots.items())
                {
                    const auto variant = std::atoi(slot.c_str());

                    if (variant < 1 || variant > 7
                        || !edges.is_string())
                    {
                        continue;
                    }

                    connectors[enums::index(terrain)]
                        .edges[static_cast<std::size_t>(variant)] =
                        edgesFromText(
                            edges.get<std::string>());
                }
            }
        }

        return connectors;
    }

    std::optional<std::string> saveConnectorsFile(
        const std::filesystem::path &directory,
        const autotile::TerrainConnectors &connectors)
    {
        const auto path = directory / "tiles.json";

        nlohmann::json document = nlohmann::json::object();

        {
            std::ifstream in(path);

            if (in)
            {
                auto parsed =
                    nlohmann::json::parse(in, nullptr, false);

                if (!parsed.is_discarded() && parsed.is_object())
                {
                    document = std::move(parsed);
                }
            }
        }

        nlohmann::json section = nlohmann::json::object();

        for (const auto terrain :
             enums::kAll<tilemap::TerrainClass>)
        {
            nlohmann::json slots = nlohmann::json::object();

            for (std::size_t variant = 1; variant <= 7; ++variant)
            {
                const auto edges =
                    connectors[enums::index(terrain)]
                        .edges[variant];

                if (edges == autotile::kEdgeAll)
                {
                    continue;
                }

                slots[std::to_string(variant)] =
                    edgesToText(edges);
            }

            if (!slots.empty())
            {
                section[std::string(
                    tilemap::toString(terrain))] =
                    std::move(slots);
            }
        }

        if (section.empty())
        {
            document.erase("connectors");
        }
        else
        {
            document["connectors"] = std::move(section);
        }

        std::ofstream out(path);

        if (!out)
        {
            return "cannot open " + path.string();
        }

        out << document.dump(2) << '\n';

        if (!out.good())
        {
            return "cannot write " + path.string();
        }

        return std::nullopt;
    }

    std::optional<std::int32_t> variantSlotAt(const Point pixel)
    {
        if (pixel.x < 64 || pixel.x >= 96 || pixel.y < 0
            || pixel.y >= 64)
        {
            return std::nullopt;
        }

        const auto slot =
            (pixel.y / 16) * 2 + (pixel.x - 64) / 16;

        if (slot >= 7)
        {
            return std::nullopt;
        }

        return slot + 1;
    }

    std::optional<std::uint8_t> connectorHotspotAt(
        const Point pixel)
    {
        if (!variantSlotAt(pixel).has_value())
        {
            return std::nullopt;
        }

        const auto lx = (pixel.x - 64) % 16;
        const auto ly = pixel.y % 16;
        const bool midX = lx >= 5 && lx <= 10;
        const bool midY = ly >= 5 && ly <= 10;

        if (ly <= 1 && midX)
        {
            return autotile::kEdgeNorth;
        }

        if (ly >= 14 && midX)
        {
            return autotile::kEdgeSouth;
        }

        if (lx <= 1 && midY)
        {
            return autotile::kEdgeWest;
        }

        if (lx >= 14 && midY)
        {
            return autotile::kEdgeEast;
        }

        return std::nullopt;
    }

    namespace
    {
        void applyStrokePixel(
            SheetDoc &doc,
            const Point pixel,
            const PixelClass value)
        {
            if (setSheetPixel(doc.image, pixel, value))
            {
                doc.dirty = true;
                ++doc.revision;
            }
        }

        [[nodiscard]] PixelClass strokeClass(
            const EditorStore &store, const bool leftButton)
        {
            if (!leftButton)
            {
                return PixelClass::Blank;
            }

            return store.tiles.drawPaper ? PixelClass::Paper
                                         : PixelClass::Ink;
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

        if (gesture.kind == GestureKind::Press
            && store.view == EditorView::Tiles && gesture.ink
            && gesture.ctrl)
        {
            if (const auto edge =
                    connectorHotspotAt(gesture.pixel))
            {
                const auto slot = *variantSlotAt(gesture.pixel);

                tiles.connectors[enums::index(store.state.brush)]
                    .edges[static_cast<std::size_t>(slot)] ^=
                    *edge;
            }

            return;
        }

        if (gesture.kind == GestureKind::Press)
        {
            doc.undoStack.push_back(doc.image);
            doc.redoStack.clear();
            tiles.stroke = true;
            tiles.strokeInk = gesture.ink;
            applyStrokePixel(
                doc,
                gesture.pixel,
                strokeClass(store, gesture.ink));
            return;
        }

        if (gesture.kind == GestureKind::Move)
        {
            if (tiles.stroke)
            {
                applyStrokePixel(
                    doc,
                    gesture.pixel,
                    strokeClass(store, tiles.strokeInk));
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

        const auto connectorError = saveConnectorsFile(
            store.tiles.directory, store.tiles.connectors);

        if (connectorError.has_value())
        {
            logger.log(log::Level::Error, *connectorError);
        }

        logger.log(
            log::Level::Info,
            "map_editor: saved "
                + sheetPathFor(store.tiles.directory, store.state.brush)
                      .string());
    }

    namespace
    {
        void drawMarkerOutline(
            gfx::ViewportRenderer &view,
            const PointF origin,
            const float size,
            const Color color,
            const float inset)
        {
            const auto left = origin.x + inset;
            const auto top = origin.y + inset;
            const auto extent = size - 2.0F * inset;

            view.drawRect(
                RectF({left, top}, {extent, 1.0F}), color);
            view.drawRect(
                RectF(
                    {left, top + extent - 1.0F}, {extent, 1.0F}),
                color);
            view.drawRect(
                RectF(
                    {left, top + 1.0F}, {1.0F, extent - 2.0F}),
                color);
            view.drawRect(
                RectF(
                    {left + extent - 1.0F, top + 1.0F},
                    {1.0F, extent - 2.0F}),
                color);
        }
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
        const autotile::SheetConnectors &connectors,
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
                x % 16 == 0 ? kBandGuideColor : kSlotGuideColor);
        }

        for (std::int32_t y = 0;
             y <= static_cast<std::int32_t>(kSheetHeight);
             y += 8)
        {
            const auto lineY = top + static_cast<float>(y) * zoom;

            view.drawLine(
                {left, lineY},
                {left + static_cast<float>(kWorkspaceWidth), lineY},
                y % 16 == 0 ? kBandGuideColor : kSlotGuideColor);
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

        if (const auto slot = variantSlotAt(*hover))
        {
            const auto tileX = static_cast<float>(
                64 + ((hover->x - 64) / 16) * 16);
            const auto tileY =
                static_cast<float>((hover->y / 16) * 16);
            const auto edges = connectors.edges[static_cast<
                std::size_t>(*slot)];
            const std::array<
                std::pair<std::uint8_t, PointF>,
                4>
                markers{
                    {{autotile::kEdgeNorth, {7.0F, 0.0F}},
                     {autotile::kEdgeSouth, {7.0F, 14.0F}},
                     {autotile::kEdgeWest, {0.0F, 7.0F}},
                     {autotile::kEdgeEast, {14.0F, 7.0F}}}};

            for (const auto &[bit, at] : markers)
            {
                const auto on = (edges & bit) != 0;
                const auto color =
                    on ? Color{
                             .red = 244,
                             .green = 208,
                             .blue = 63,
                             .alpha = 220}
                       : Color{
                             .red = 110,
                             .green = 114,
                             .blue = 124,
                             .alpha = 180};
                const PointF origin{
                    left + (tileX + at.x) * zoom,
                    top + (tileY + at.y) * zoom};
                const auto size = 2.0F * zoom;

                drawMarkerOutline(
                    view,
                    origin,
                    size,
                    Color{.red = 0, .green = 0, .blue = 0,
                          .alpha = 200},
                    1.0F);
                drawMarkerOutline(view, origin, size, color, 0.0F);
            }
        }
    }

}
