#pragma once

#include <filesystem>
#include <optional>
#include <string>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/autotile/Connectors.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/ViewportRenderer.hpp>
#include <antwika/log/ILogger.hpp>
#include <antwika/tilemap/TerrainClass.hpp>

#include "antwika/map_editor/EditorStore.hpp"

namespace antwika::map_editor
{

    inline constexpr std::int32_t kSheetZoom = 3;

    /**
     * @brief The zoom at or above which the faint pixel grid shows.
     *
     * The workspace zoom is fixed at kSheetZoom, which sits at
     * this gate, mirroring atlas_editor's zoom-gated grid.
     */
    inline constexpr std::int32_t kPixelGridMinZoom = 3;

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

    enum class PixelClass : std::uint8_t
    {
        Blank = 0,
        Ink,
        Paper,
    };

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
     * @brief Loads a terrain sheet from the tiles directory.
     *
     * @return The file's bitmap when it exists and is exactly
     *         96x64, else the procedural placeholder.
     *
     * Ensures: a legacy 32x48 sheet logs that a redraw is needed
     *          and falls back to the placeholder.
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
     * @brief Loads the per-variant edge connectors from tiles.json.
     *
     * Ensures: terrains or variants the sidecar does not mention
     *          keep all four edges connected, reproducing the
     *          pre-connector behavior.
     */
    [[nodiscard]] autotile::TerrainConnectors loadConnectorsFile(
        const std::filesystem::path &directory);

    /**
     * @brief Writes every terrain's connectors into tiles.json.
     *
     * @return An error message, or nothing on success.
     *
     * Ensures: other sidecar members are preserved, and all-edge
     *          defaults are omitted so untouched sheets keep a
     *          connector-free sidecar.
     */
    [[nodiscard]] std::optional<std::string> saveConnectorsFile(
        const std::filesystem::path &directory,
        const autotile::TerrainConnectors &connectors);

    /**
     * @brief The variant slot under a sheet pixel, if any.
     *
     * @return The variant id 1 to 7 for the seven variant slots,
     *         and nothing elsewhere (the frame B slot included).
     */
    [[nodiscard]] std::optional<std::int32_t> variantSlotAt(
        gfx::Point pixel);

    /**
     * @brief The connector edge under a variant-slot pixel, if any.
     *
     * @return One edge bit when the pixel sits in a variant tile's
     *         edge-midpoint zone: six pixels along the edge and two
     *         pixels deep.
     *
     * Ensures: only a ctrl-click consults this zone, so plain
     *          clicks paint everywhere on the tile.
     */
    [[nodiscard]] std::optional<std::uint8_t> connectorHotspotAt(
        gfx::Point pixel);

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
     * kSheetZoom, the pixel grid, the 8x8 slot guides, and the
     * hover highlight; the hover label lives in the hint line.
     */
    void drawSheetWorkspace(
        gfx::ViewportRenderer &view,
        const gfx::ITexture &sheet,
        const autotile::SheetConnectors &connectors,
        std::optional<gfx::Point> hover);

}
