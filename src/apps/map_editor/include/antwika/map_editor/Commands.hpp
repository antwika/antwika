#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>

#include <antwika/log/ILogger.hpp>
#include <antwika/tilemap/Entities.hpp>
#include <antwika/tilemap/Rgb.hpp>
#include <antwika/tilemap/TerrainClass.hpp>

#include "antwika/map_editor/Components.hpp"
#include "antwika/map_editor/EditorState.hpp"
#include "antwika/map_editor/EditorStore.hpp"

namespace antwika::map_editor
{

    inline constexpr std::int32_t kExtendMargin = 3;

    struct ExtendResult final
    {
        geometry::GridCell landed{};
        std::uint32_t west = 0;
        std::uint32_t north = 0;
    };

    /**
     * @brief Grows the map so a just-off-the-edge cell exists.
     *
     * @param target The signed cell, negative west or north of the
     *        map.
     * @return The landed cell in the grown map plus the west and
     *         north growth, or nothing when the target lies farther
     *         than kExtendMargin outside the bounds.
     *
     * Ensures: one undo snapshot covers the growth and the paint
     *          applied through paintExtended afterwards, the pin
     *          grid grows in sync with new cells free, and entities
     *          shift with the cells.
     */
    [[nodiscard]] std::optional<ExtendResult> extendMapFor(
        EditorState &state, SignedCell target);

    /**
     * @brief Applies the brush at the hovered cell with no snapshot.
     *
     * Ensures: the caller already holds the undo snapshot, so the
     *          growth and the paint revert together.
     */
    void paintExtended(EditorState &state);

    void selectBrush(EditorState &state, tilemap::TerrainClass terrain);

    void selectFreeBrush(EditorState &state);

    void paintHovered(EditorState &state);

    /**
     * @brief Writes generated terrains into the unpinned cells.
     *
     * @param terrains One terrain per map cell in row-major order.
     *
     * Ensures: pinned cells and every non-terrain attribute keep
     *          their values, and one undo snapshot is taken.
     */
    void applyGenerated(
        EditorState &state,
        const std::vector<tilemap::TerrainClass> &terrains);

    void raiseHovered(EditorState &state);

    void lowerHovered(EditorState &state);

    void toggleBridge(EditorState &state);

    void cycleLight(EditorState &state);

    void placeTransition(EditorState &state);

    void placeNpc(EditorState &state);

    void placePickup(EditorState &state);

    void placeEntityKind(EditorState &state, MarkerKind kind);

    /**
     * @brief Replaces one map entity with an edited copy.
     *
     * @param index The position in the map's entity list.
     * @param entity The replacement written at that position.
     *
     * Ensures: an out-of-range index leaves the map untouched.
     */
    void replaceEntity(
        EditorState &state, std::size_t index, tilemap::Entity entity);

    void removeEntitiesAtHovered(EditorState &state);

    void markStampStart(EditorState &state);

    void copyStampEnd(EditorState &state);

    void pasteStamp(EditorState &state);

    /**
     * @brief Sets the header palette as one undoable edit.
     *
     * Ensures: an unchanged palette takes no undo snapshot, and
     *          cells, entities, and the rest of the header keep
     *          their values.
     */
    void setPalette(
        EditorState &state, tilemap::Rgb ink, tilemap::Rgb paper);

    /**
     * @brief Sets the header palette with no undo snapshot.
     *
     * Ensures: cells, entities, and the rest of the header keep
     *          their values, so the caller can restore the prior
     *          palette the same way.
     */
    void previewPalette(
        EditorState &state, tilemap::Rgb ink, tilemap::Rgb paper);

    /**
     * @brief Opens the palette dialog on the ink swatch.
     *
     * Ensures: the header palette from before the dialog is kept
     *          for Cancel, and the picker state matches the ink
     *          color.
     */
    void openPaletteDialog(EditorStore &store);

    /**
     * @brief Commits the previewed palette as one undoable edit.
     *
     * Ensures: a single undo reverts both colors together, and the
     *          dialog closes.
     */
    void applyPaletteDialog(EditorStore &store);

    /**
     * @brief Closes the palette dialog and restores the colors.
     *
     * Ensures: the header palette returns to its values from before
     *          the dialog opened, with no undo entry.
     */
    void cancelPaletteDialog(EditorStore &store);

    /**
     * @brief Reloads the picker state from the active swatch.
     *
     * Ensures: the hue, saturation, value, and hex text match the
     *          active swatch's current color.
     */
    void syncPaletteFromActive(EditorStore &store);

    /**
     * @brief The color of the palette dialog's active swatch.
     */
    [[nodiscard]] tilemap::Rgb activePaletteColor(
        const EditorStore &store);

    /**
     * @brief Previews a picked color on the active swatch.
     *
     * Ensures: the hex field text follows the color unless the
     *          caller is echoing the user's own hex input.
     */
    void pickPaletteColor(
        EditorStore &store, tilemap::Rgb color, bool refreshHex);

    void undo(EditorState &state);

    void redo(EditorState &state);

    void toggleOverlay(EditorState &state);

    void saveMap(EditorState &state, log::ILogger &logger);

    /**
     * @brief Loads the map at the given path as an undoable edit.
     *
     * @return An error message, or nothing on success.
     *
     * Ensures: on success the path becomes the current one and every
     *          cell is pinned; on failure nothing changes.
     */
    [[nodiscard]] std::optional<std::string> openMapAt(
        EditorState &state,
        const std::filesystem::path &path,
        log::ILogger &logger);

    /**
     * @brief Saves the map to the given path.
     *
     * @return An error message, or nothing on success.
     *
     * Ensures: on success the path becomes the current one.
     */
    [[nodiscard]] std::optional<std::string> saveMapAt(
        EditorState &state,
        const std::filesystem::path &path,
        log::ILogger &logger);

    void playtest(EditorState &state, log::ILogger &logger);

    void reloadMap(EditorState &state, log::ILogger &logger);

    /**
     * @brief Recomputes the validator report immediately.
     *
     * Ensures: the refresh throttle is bypassed and its counter
     *          restarts.
     */
    void validateNow(EditorState &state);

    /**
     * @brief Refreshes the validator report when it is due.
     *
     * Ensures: the report recomputes only after an edit, and at most
     *          once every thirty frames while one already exists.
     */
    void refreshReport(EditorState &state);

}
