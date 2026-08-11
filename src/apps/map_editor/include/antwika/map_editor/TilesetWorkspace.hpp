#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/ViewportRenderer.hpp>
#include <antwika/log/ILogger.hpp>
#include <antwika/tileset/Atlas.hpp>
#include <antwika/tileset/Sprite.hpp>

#include "antwika/map_editor/EditorStore.hpp"
#include "antwika/map_editor/TilesetPreview.hpp"

namespace antwika::map_editor
{

    inline constexpr std::int32_t kTilesetEditorZoom = 16;

    inline constexpr std::int32_t kTilesetEditorLeft = 20;

    inline constexpr std::int32_t kTilesetEditorTop = 38;

    inline constexpr std::int32_t kLibraryLeft = 184;

    inline constexpr std::int32_t kLibraryTop = 26;

    inline constexpr std::size_t kLibraryColumns = 6;

    inline constexpr std::size_t kLibraryRows = 11;

    inline constexpr std::int32_t kLibraryPitch = 20;

    inline constexpr std::size_t kLibraryPageSize =
        kLibraryColumns * kLibraryRows;

    inline constexpr std::size_t kMaxNamedSockets = 12;

    /**
     * @brief Finds the sprite pixel under a canvas position.
     *
     * @return The 8x8 editor pixel, or nothing outside the editor.
     */
    [[nodiscard]] std::optional<gfx::Point> editorPixelAt(
        gfx::Point canvas) noexcept;

    /**
     * @brief Finds the socket band under a canvas position.
     */
    [[nodiscard]] std::optional<tileset::Side> socketBandAt(
        gfx::Point canvas) noexcept;

    /**
     * @brief Finds the library cell under a canvas position.
     *
     * @return The cell index within the visible page, or nothing
     *         outside the grid's cell art.
     */
    [[nodiscard]] std::optional<std::size_t> libraryCellAt(
        gfx::Point canvas) noexcept;

    /**
     * @brief Finds the frame preview under a canvas position.
     */
    [[nodiscard]] std::optional<std::size_t> framePreviewAt(
        gfx::Point canvas) noexcept;

    [[nodiscard]] bool overLibrary(gfx::Point canvas) noexcept;

    [[nodiscard]] bool overPreview(gfx::Point canvas) noexcept;

    [[nodiscard]] bool overPreviewRegen(gfx::Point canvas) noexcept;

    [[nodiscard]] bool overPreviewAuto(gfx::Point canvas) noexcept;

    /**
     * @brief The fill color a socket's bands and ticks take.
     *
     * Ensures: the reserved open socket is dark gray, and named
     *          sockets cycle a deterministic twelve-color palette by
     *          id; the reserved edge socket draws as a hatch and its
     *          base color here is black.
     */
    [[nodiscard]] gfx::Color socketColor(
        tileset::SocketId socket) noexcept;

    /**
     * @brief Pages the library grid while clamping to its content.
     */
    void adjustLibraryPage(EditorStore &store, std::int32_t delta);

    /**
     * @brief Switches the active open tileset.
     *
     * Ensures: an index past the open list is ignored, and a switch
     *          resets the library page, the active socket, the
     *          delete confirmation, and the workspace message.
     */
    void activateTileset(EditorStore &store, std::size_t index);

    /**
     * @brief Takes an undo snapshot of the active tileset.
     *
     * Ensures: the redo stack clears and the stack keeps at most
     *          sixty-four snapshots, dropping the oldest.
     */
    void pushTilesetSnapshot(TilesetDoc &doc);

    /**
     * @brief Folds one pointer gesture into the tiles workspace.
     *
     * @param gesture A gesture whose pixel member holds the raw
     *        canvas position.
     *
     * Ensures: a press starts an undoable pixel stroke or decor
     *          toggle drag, and a release that changed nothing
     *          leaves the undo stack untouched.
     */
    void applyTilesetGesture(
        EditorStore &store, const SheetGesture &gesture);

    void tilesetUndo(EditorStore &store);

    void tilesetRedo(EditorStore &store);

    void selectTilesetFrame(EditorStore &store, std::size_t frame);

    /**
     * @brief Applies the Clr button to the selected frame.
     *
     * Ensures: frame one clears its pixels, a later frame deletes
     *          itself and every frame after it, the selection falls
     *          back to an existing frame, and the change is one undo
     *          step.
     */
    void clearActiveFrame(EditorStore &store);

    void addLayerPressed(EditorStore &store);

    void removeLayerPressed(EditorStore &store);

    void addSpritePressed(EditorStore &store);

    void duplicateSpritePressed(EditorStore &store);

    void deleteSpriteConfirmed(EditorStore &store);

    void addSocketPressed(EditorStore &store);

    void renameSocketPressed(EditorStore &store);

    /**
     * @brief Deletes the active named socket.
     *
     * Ensures: a reserved socket or one held by any sprite edge in
     *          any layer is refused with a message, and sprite
     *          socket ids above the removed one shift down to keep
     *          their names.
     */
    void deleteSocketPressed(EditorStore &store);

    void setDecorAll(EditorStore &store, bool allowed);

    void adjustDensity(EditorStore &store, std::int32_t delta);

    /**
     * @brief Steps the selected sprite's weight.
     *
     * Ensures: the weight stays between tileset::kMinWeight and
     *          tileset::kMaxWeight, and a change is one undo step.
     */
    void adjustWeight(EditorStore &store, std::int32_t delta);

    void createTilesetPressed(EditorStore &store);

    void saveActiveTileset(EditorStore &store, log::ILogger &logger);

    /**
     * @brief Draws the tiles-view left workspace.
     *
     * Draws the caption, the magnified sprite editor with its socket
     * bands, the frame strip with an animated preview, the library
     * grid, and the generated-combination preview panel, all from
     * the active tileset's baked atlas.
     *
     * @param preview The generated combination to show, or null to
     *        leave the preview lattice as bare checker.
     *
     * Ensures: a tileset holding no layers draws nothing at all.
     */
    void drawTilesetWorkspace(
        gfx::ViewportRenderer &view,
        const EditorStore &store,
        const gfx::ITexture *atlas,
        const tileset::AtlasIndex &index,
        std::uint32_t tick,
        const TilesetPreview *preview);

}
