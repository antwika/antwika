#include "antwika/map_editor/SheetWorkspace.hpp"

#include <cstddef>
#include <cstdint>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/ui/Theme.hpp>

namespace antwika::map_editor
{

    namespace
    {
        using antwika::gfx::Bitmap;
        using antwika::gfx::Color;
        using antwika::gfx::Point;
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

            return store.tilesets.drawPaper ? PixelClass::Paper
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
        auto &tilesets = store.tilesets;

        if (gesture.kind == GestureKind::Press)
        {
            doc.undoStack.push_back(doc.image);
            doc.redoStack.clear();
            tilesets.stroke = true;
            tilesets.strokeInk = gesture.ink;
            applyStrokePixel(
                doc,
                gesture.pixel,
                strokeClass(store, gesture.ink));
            return;
        }

        if (gesture.kind == GestureKind::Move)
        {
            if (tilesets.stroke)
            {
                applyStrokePixel(
                    doc,
                    gesture.pixel,
                    strokeClass(store, tilesets.strokeInk));
            }

            return;
        }

        if (!tilesets.stroke)
        {
            return;
        }

        tilesets.stroke = false;

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

}
