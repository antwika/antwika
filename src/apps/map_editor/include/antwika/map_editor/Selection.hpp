#pragma once

#include <optional>

#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/ViewportRenderer.hpp>
#include <antwika/input/Key.hpp>

#include "antwika/map_editor/EditorStore.hpp"

namespace antwika::map_editor
{

    /**
     * @brief The cell rectangle spanned by two marquee corners.
     */
    [[nodiscard]] CellSpan cellSpanOf(
        geometry::GridCell a, geometry::GridCell b);

    /**
     * @brief The pixel rectangle spanned by two marquee corners.
     */
    [[nodiscard]] PixelSpan pixelSpanOf(gfx::Point a, gfx::Point b);

    [[nodiscard]] bool cellSpanContains(
        const CellSpan &span, geometry::GridCell cell);

    [[nodiscard]] bool pixelSpanContains(
        const PixelSpan &span, gfx::Point pixel);

    /**
     * @brief The map selection clipped to the map bounds.
     *
     * @return The clipped span, or nothing when no selection is
     *         placed or the clip leaves no cells.
     */
    [[nodiscard]] std::optional<CellSpan> mapSelectionSpan(
        const EditorStore &store);

    /**
     * @brief The tiles selection when its context is still live.
     *
     * @return The pixel span, or nothing when no selection is
     *         placed or the active document, layer, sprite, or
     *         frame changed since the marquee.
     */
    [[nodiscard]] std::optional<PixelSpan> tilesSelectionSpan(
        const EditorStore &store);

    /**
     * @brief The character selection when its context is live.
     *
     * @return The pixel span, or nothing when no selection is
     *         placed or the selected character changed since the
     *         marquee.
     */
    [[nodiscard]] std::optional<PixelSpan> charSelectionSpan(
        const EditorStore &store);

    /**
     * @brief Folds one map gesture into the select tool.
     *
     * Ensures: a press inside the placed selection starts a move, a
     *          press elsewhere starts a marquee, a release without a
     *          drag clears the selection, and a move release applies
     *          the displacement as one undo step.
     */
    void applyMapSelectGesture(
        EditorStore &store, const MapGesture &gesture);

    /**
     * @brief Folds one tiles gesture into the select tool.
     *
     * @param gesture A gesture whose pixel member holds the raw
     *        canvas position.
     */
    void applyTilesSelectGesture(
        EditorStore &store, const SheetGesture &gesture);

    /**
     * @brief Folds one character-sheet gesture into the select tool.
     *
     * @param gesture A gesture whose pixel member holds the sheet
     *        pixel.
     */
    void applyCharSelectGesture(
        EditorStore &store, const SheetGesture &gesture);

    /**
     * @brief Runs the Ctrl chord for the current view.
     *
     * @return True for C, X, and V, whether or not a selection or
     *         clipboard made the chord act, so plain-key bindings
     *         never fire under Ctrl.
     */
    bool selectionChord(EditorStore &store, input::Key key);

    void copySelection(EditorStore &store);

    /**
     * @brief Copies, then empties, the selection as one undo step.
     *
     * Ensures: cut map columns lose every slab and become pinned,
     *          and cut workspace pixels become blank.
     */
    void cutSelection(EditorStore &store);

    /**
     * @brief Pastes the view's clipboard at the hovered position.
     *
     * Ensures: the clipboard's top-left lands on the hovered cell
     *          or pixel, the paste clips at the bounds, map columns
     *          paste level-absolute, blank clipboard pixels leave
     *          the target pixel untouched, and the paste is one
     *          undo step.
     */
    void pasteClipboard(EditorStore &store);

    /**
     * @brief Clears the current view's selection.
     *
     * @return True when there was a live selection to clear.
     */
    bool clearActiveSelection(EditorStore &store);

    /**
     * @brief Leaves the select tool for the view's default tool.
     *
     * @return True when the current view's select tool was active.
     */
    bool exitActiveSelectTool(EditorStore &store);

    /**
     * @brief Drops every view's selection after an undo or redo.
     */
    void clearSelectionsAfterHistory(EditorStore &store);

    /**
     * @brief Draws the tiles-view marquee at the editor zoom.
     *
     * Ensures: a marquee in progress draws dashed, a placed
     *          selection draws solid, and a move in progress draws
     *          the outline displaced by the drag delta.
     */
    void drawTilesSelectionOverlay(
        gfx::ViewportRenderer &view, const EditorStore &store);

    /**
     * @brief Draws the characters-view marquee at the sheet zoom.
     *
     * Ensures: a marquee in progress draws dashed, a placed
     *          selection draws solid, and a move in progress draws
     *          the outline displaced by the drag delta.
     */
    void drawCharSelectionOverlay(
        gfx::ViewportRenderer &view, const EditorStore &store);

}
