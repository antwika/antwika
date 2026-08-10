#pragma once

#include <filesystem>
#include <optional>
#include <string>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/ViewportRenderer.hpp>
#include <antwika/log/ILogger.hpp>
#include <antwika/tilemap/TerrainClass.hpp>

#include "antwika/map_editor/EditorStore.hpp"

namespace antwika::map_editor
{

    inline constexpr std::int32_t kSheetZoom = 5;

    /**
     * @brief The zoom at or above which the faint pixel grid shows.
     *
     * The workspace zoom is fixed at kSheetZoom, which sits above
     * this gate, mirroring atlas_editor's zoom-gated grid.
     */
    inline constexpr std::int32_t kPixelGridMinZoom = 4;

    /**
     * @brief Finds the sheet pixel under a canvas position.
     *
     * @param canvas The pointer position in canvas pixels.
     * @return The sheet pixel, or nothing outside the sheet.
     */
    [[nodiscard]] std::optional<gfx::Point> sheetPixelAt(
        gfx::Point canvas) noexcept;

    /**
     * @brief Names the sheet slot holding the given pixel.
     *
     * @return A label such as "mask 5" or "bridge deck", following
     *         docs/TILE_SHEETS.md.
     */
    [[nodiscard]] std::string slotLabelAt(gfx::Point pixel);

    /**
     * @brief Writes one 1-bit pixel into a sheet bitmap.
     *
     * @param ink True paints the ink color and false clears the
     *        pixel to transparent.
     * @return True when the pixel changed.
     */
    bool setSheetPixel(gfx::Bitmap &sheet, gfx::Point pixel, bool ink);

    /**
     * @brief Reads whether a sheet pixel currently holds ink.
     */
    [[nodiscard]] bool sheetPixelInked(
        const gfx::Bitmap &sheet, gfx::Point pixel);

    /**
     * @brief Loads a terrain sheet from the tiles directory.
     *
     * @return The file's bitmap when it exists and is exactly
     *         32x48, else the procedural placeholder.
     */
    [[nodiscard]] gfx::Bitmap loadSheetOrPlaceholder(
        const std::filesystem::path &directory,
        tilemap::TerrainClass terrain,
        log::ILogger &logger);

    /**
     * @brief Writes a sheet to <directory>/<terrain>.png.
     *
     * @return An error message, or nothing on success.
     */
    [[nodiscard]] std::optional<std::string> saveSheet(
        const gfx::Bitmap &sheet,
        const std::filesystem::path &directory,
        tilemap::TerrainClass terrain);

    [[nodiscard]] std::filesystem::path sheetPathFor(
        const std::filesystem::path &directory,
        tilemap::TerrainClass terrain);

    /**
     * @brief Folds one pointer gesture into the current sheet.
     *
     * Ensures: a press starts an undoable stroke, moves extend it
     *          with the press's ink state, and a release that
     *          changed nothing leaves the undo stack untouched.
     */
    void applySheetGesture(
        EditorStore &store, const SheetGesture &gesture);

    void sheetUndo(EditorStore &store);

    void sheetRedo(EditorStore &store);

    /**
     * @brief Saves the selected terrain's sheet to its PNG.
     *
     * Ensures: success clears the document's dirty flag and failure
     *          is logged.
     */
    void saveActiveTerrainSheet(
        EditorStore &store, log::ILogger &logger);

    /**
     * @brief Draws the magnified pixel workspace for one sheet.
     *
     * Draws the transparency backdrop, the sheet texture at
     * kSheetZoom, the pixel grid, the 8x8 slot guides, the hover
     * highlight, and the hover label with an ink indicator.
     */
    void drawSheetWorkspace(
        gfx::ViewportRenderer &view,
        const gfx::ITexture &sheet,
        const gfx::Bitmap &image,
        std::optional<gfx::Point> hover);

}
