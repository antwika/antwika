#pragma once

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/ViewportRenderer.hpp>
#include <antwika/tileset/PixelClass.hpp>

#include "antwika/map_editor/EditorStore.hpp"

namespace antwika::map_editor
{

    using tileset::PixelClass;

    /**
     * @brief The zoom at or above which the faint pixel grid shows.
     */
    inline constexpr std::int32_t kPixelGridMinZoom = 3;

    /**
     * @brief Writes one classed pixel into a sheet bitmap.
     *
     * @param value Ink stores opaque white, Paper stores opaque
     *        mid-gray, and Blank clears to transparency.
     * @return True when the pixel changed.
     */
    bool setSheetPixel(
        gfx::Bitmap &sheet, gfx::Point pixel, PixelClass value);

    /**
     * @brief Normalizes every opaque pixel to its class color.
     *
     * Ensures: opaque pixels with luminance at or above 192 store
     *          the ink white and the rest store the paper mid-gray,
     *          so legacy all-white and off-white sheets load as
     *          all-ink.
     */
    void normalizeSheetClasses(gfx::Bitmap &sheet);

    /**
     * @brief Reads a sheet pixel's class.
     *
     * Ensures: transparent pixels read Blank, and opaque pixels
     *          split by the 192 luminance threshold.
     */
    [[nodiscard]] PixelClass sheetPixelClass(
        const gfx::Bitmap &sheet, gfx::Point pixel);

    /**
     * @brief Composes a drawable bitmap from a classed sheet.
     *
     * @return The sheet with ink-class pixels colored by ink and
     *         paper-class pixels by paper, alpha preserved.
     */
    [[nodiscard]] gfx::Bitmap bakedSheet(
        const gfx::Bitmap &sheet, gfx::Color ink, gfx::Color paper);

    /**
     * @brief Outlines one magnified workspace pixel in yellow.
     *
     * @param origin The pixel's top-left corner in canvas pixels.
     * @param zoom The workspace magnification in canvas pixels.
     *
     * Ensures: the outline is the theme's focus-ring yellow, one
     *          canvas pixel thin, drawn just inside the pixel.
     */
    void drawPixelOutline(
        gfx::ViewportRenderer &view, gfx::PointF origin, float zoom);

    /**
     * @brief Folds one pointer gesture into the character sheet.
     *
     * Ensures: a press starts an undoable stroke, moves extend it
     *          with the press's ink state, and a release that
     *          changed nothing leaves the undo stack untouched.
     */
    void applySheetGesture(
        EditorStore &store, const SheetGesture &gesture);

    void sheetUndo(EditorStore &store);

    void sheetRedo(EditorStore &store);

}
