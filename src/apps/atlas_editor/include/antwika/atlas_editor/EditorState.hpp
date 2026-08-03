#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/atlas_editor/Canvas.hpp"
#include "antwika/atlas_editor/CanvasView.hpp"
#include "antwika/atlas_editor/Pixel.hpp"
#include "antwika/atlas_editor/Selection.hpp"
#include "antwika/atlas_editor/SpriteGuides.hpp"
#include "antwika/atlas_editor/StatusMessage.hpp"
#include "antwika/atlas_editor/TileGrid.hpp"
#include "antwika/atlas_editor/Tool.hpp"

namespace antwika::atlas_editor
{

    using antwika::gfx::Color;
    using antwika::gfx::Point;
    using antwika::gfx::Size;

    /**
     * @brief Everything one editing session is, apart from the picture
     * of it.
     *
     * All of it is regenerated from the recorded input on a replay --
     * the image, the view, the tool, the colour and the counts alike --
     * which is why none of it is ever persisted as an event and why
     * nothing here may be read from a channel a replay does not carry.
     *
     * It opens no files and knows nothing about one: what a save and a
     * load *are* is IAtlasStore's, and this only takes the image one
     * hands back and counts that it happened.
     */
    class EditorState final
    {
    public:
        /**
         * @brief Open a session over an image.
         * @param image The sheet being edited.
         * @param tiles How to divide it into slots for the grid overlay.
         * @param canvas The size the window was asked for, which
         * everything is laid out and hit-tested against.
         */
        EditorState(Canvas image, TileGrid tiles, Size canvas);

        /**
         * @brief Get the image being edited.
         * @return The canvas.
         */
        [[nodiscard]] const Canvas &image() const noexcept;

        /**
         * @brief Get where the image sits on the canvas.
         * @return The view.
         */
        [[nodiscard]] CanvasView view() const noexcept;

        /**
         * @brief Get the area everything is laid out against.
         * @return The canvas size this session was opened over.
         */
        [[nodiscard]] Size canvas() const noexcept;

        /**
         * @brief Get how the sheet is divided into slots.
         * @return The tile grid.
         */
        [[nodiscard]] TileGrid tiles() const noexcept;

        /**
         * @brief Get what a left click on the image does.
         * @return The selected tool.
         */
        [[nodiscard]] Tool tool() const noexcept;

        /**
         * @brief Get the colour Paint puts down.
         * @return The selected colour.
         */
        [[nodiscard]] Color color() const noexcept;

        /**
         * @brief Get which swatch is selected, if any.
         * @return The palette index, or nothing after a Pick took a
         * colour the palette does not offer.
         */
        [[nodiscard]] std::optional<std::size_t>
        colorIndex() const noexcept;

        /**
         * @brief Check whether the slot grid is being drawn.
         * @return True while it is.
         */
        [[nodiscard]] bool gridVisible() const noexcept;

        /**
         * @brief Check whether the sprite guides are being drawn.
         * @return True while they are, whether or not this session's
         * slot size has any to draw.
         */
        [[nodiscard]] bool guidesVisible() const noexcept;

        /**
         * @brief Get where a slot puts its footprint diamond.
         *
         * Derived once from the slot size this session was opened with,
         * since nothing changes that: a load replaces the sheet and
         * never the grid over it.
         *
         * @return The guides, or nothing when the slot size is not one
         * an isometric sprite's shape comes out of.
         */
        [[nodiscard]] std::optional<SpriteGuides> guides() const noexcept;

        /**
         * @brief Get which image pixel the pointer was last over.
         * @return The pixel, or nothing until an event has said where
         * the pointer is -- the folded default is the canvas's corner,
         * which is a pixel of the sheet and would read as a real hover.
         */
        [[nodiscard]] std::optional<Pixel> hovered() const noexcept;

        /**
         * @brief Get the rectangle that has been marked out, if any.
         *
         * What cut and copy act on, and what a drag from inside moves.
         * It is what the *last finished* gesture left, so it does not
         * change under a drag in progress -- see shownSelection().
         *
         * @return The selection, or nothing when none is marked.
         */
        [[nodiscard]] std::optional<Selection> selection() const noexcept;

        /**
         * @brief Get the rectangle to draw an outline round.
         *
         * The one a drag in progress is heading for, when there is one,
         * and the marked one otherwise -- so the outline follows the
         * pointer while a rectangle is being drawn or carried, and the
         * sheet itself changes only when the button comes up.
         *
         * @return What to outline, or nothing when there is nothing to.
         */
        [[nodiscard]] std::optional<Selection>
        shownSelection() const noexcept;

        /**
         * @brief Check whether anything has been cut or copied.
         * @return True once there is something a paste would put down.
         */
        [[nodiscard]] bool hasClipboard() const noexcept;

        /**
         * @brief Get the last thing worth telling the artist.
         * @return The message, or nothing until something has
         * happened.
         */
        [[nodiscard]] const std::optional<StatusMessage> &status()
            const noexcept;

        /**
         * @brief Get how many pixels this session has changed.
         * @return The count, which a drag over one pixel adds one to.
         */
        [[nodiscard]] std::uint64_t edits() const noexcept;

        /**
         * @brief Get how many ticks this session has run for.
         * @return The count.
         */
        [[nodiscard]] std::uint64_t ticks() const noexcept;

        /**
         * @brief Get how many times the sheet has been written out.
         * @return The count.
         */
        [[nodiscard]] std::uint32_t saves() const noexcept;

        /**
         * @brief Get how many times a sheet has been read in.
         * @return The count.
         */
        [[nodiscard]] std::uint32_t loads() const noexcept;

        /**
         * @brief Check whether the image has changed since it was last
         * written out.
         * @return True when there is something worth saving.
         */
        [[nodiscard]] bool unsaved() const noexcept;

        /**
         * @brief Choose what a left click does.
         * @param tool The tool to select.
         */
        void selectTool(Tool tool) noexcept;

        /**
         * @brief Choose the colour Paint puts down.
         * @param index Which swatch of the default palette; one the
         * palette does not offer is ignored, since a toolbar that has
         * drifted from the palette should not repaint a sheet in
         * whatever happened to be next to it in memory.
         */
        void selectColor(std::size_t index) noexcept;

        /**
         * @brief Show or hide the slot grid.
         */
        void toggleGrid() noexcept;

        /**
         * @brief Show or hide the sprite guides.
         */
        void toggleGuides() noexcept;

        /**
         * @brief Zoom in one step, keeping the pixel under a point put.
         * @param anchor The canvas position to keep still.
         */
        void zoomIn(Point anchor) noexcept;

        /**
         * @brief Zoom out one step, keeping the pixel under a point put.
         * @param anchor The canvas position to keep still.
         */
        void zoomOut(Point anchor) noexcept;

        /**
         * @brief Slide the image across the canvas.
         * @param by How far to slide it, in canvas pixels.
         */
        void panBy(Point by) noexcept;

        /**
         * @brief Put the whole sheet back in the middle of the canvas.
         */
        void resetView() noexcept;

        /**
         * @brief Note where the pointer is.
         * @param point The canvas position it reported.
         */
        void moveTo(Point point) noexcept;

        /**
         * @brief Do what the selected tool does, at a canvas position.
         *
         * Not noexcept, because Fill walks a region of unknown size and
         * needs somewhere to keep the pixels it has still to look at.
         * The alternative was recursion, which on the game's own 1x1
         * sheet is a quarter of a million stack frames.
         *
         * @param point Where the pointer was; one outside the image
         * changes nothing, since a drag off the edge of the sheet is
         * ordinary input rather than an error.
         */
        void applyAt(Point point);

        /**
         * @brief Make the pixel under a canvas position transparent.
         *
         * What the right button does whichever tool is selected, so that
         * taking a mistake back never costs a trip to the toolbar.
         *
         * @param point Where the pointer was.
         */
        void eraseAt(Point point);

        /**
         * @brief Begin a selection gesture at a canvas position.
         *
         * Which gesture it is falls out of where the press landed: one
         * inside the marked rectangle carries it, and one anywhere else
         * starts drawing a new rectangle from that corner.
         *
         * @param point Where the button went down.
         */
        void beginSelecting(Point point) noexcept;

        /**
         * @brief Carry a selection gesture to a canvas position.
         *
         * Changes no pixel: what a drag moves is the outline, and the
         * sheet is only ever written when the button comes up.
         *
         * @param point Where the pointer is now; a call with no gesture
         * in progress does nothing.
         */
        void dragSelectionTo(Point point) noexcept;

        /**
         * @brief Finish a selection gesture at a canvas position.
         *
         * A drawn rectangle becomes the marked one; a carried one takes
         * its pixels with it, clearing where they came from.
         *
         * @param point Where the button came up.
         */
        void finishSelecting(Point point);

        /**
         * @brief Drop the marked rectangle and any gesture on it.
         */
        void clearSelection() noexcept;

        /**
         * @brief Take a copy of the marked rectangle's pixels.
         *
         * Nothing marked is nothing to copy, and the clipboard is left
         * holding whatever it already had rather than emptied: a copy
         * that missed should not also lose what was in hand.
         */
        void copySelection();

        /**
         * @brief Take the marked rectangle's pixels, clearing them.
         */
        void cutSelection();

        /**
         * @brief Put the clipboard down with its corner under the
         * pointer, and mark out where it landed.
         *
         * At the pointer rather than where it was taken from, because
         * the whole of what this is for is carrying art from one slot to
         * another -- and the pixel under the pointer is state a replay
         * regenerates, this being the one application that records every
         * movement rather than thinning them out.
         */
        void pasteClipboard();

        /**
         * @brief Take an image somebody loaded, in place of this one.
         *
         * The view is recentred rather than kept: a sheet of another
         * size at the old pan can be entirely off the window, and an
         * editor that appears to have loaded nothing is worse than one
         * that moves the view.
         *
         * @param image The image to edit from now on.
         */
        void replace(Canvas image);

        /**
         * @brief Note that the image has been written out as it stands.
         */
        void markSaved() noexcept;

        /**
         * @brief Note that one more tick has been stepped.
         */
        void noteTick() noexcept;

        /**
         * @brief Say what just happened, for the status line.
         * @param message Which message, and what it names.
         */
        void setStatus(StatusMessage message);

    private:
        /**
         * @brief What a left drag is doing to the selection.
         *
         * One value rather than a flag and two loose corners, so a
         * gesture cannot be half in progress: there is either one of
         * these or there is no gesture.
         */
        struct Gesture
        {
            /** @brief Whether the drag draws a rectangle or carries one. */
            bool carrying = false;

            /** @brief The pixel the button went down on. */
            Pixel from{};

            /** @brief The pixel the pointer has reached. */
            Pixel to{};
        };

        /**
         * @brief Spread the selected colour out from one pixel.
         * @param start Where the fill was asked for; one outside the
         * sheet fills nothing.
         */
        void fillFrom(Pixel start);

        /**
         * @brief Take a rectangle of the sheet as an image of its own.
         * @param area The rectangle to lift, which must be inside.
         * @return Its pixels.
         */
        [[nodiscard]] Canvas lift(const Selection &area) const;

        /**
         * @brief Write an image over the sheet at a pixel.
         *
         * Straight over, transparency included, rather than composited:
         * putting one slot's art into another means replacing what was
         * there, and a paste that let the old art show through the new
         * one's gaps could not do that at all.
         *
         * @param clip The pixels to put down.
         * @param at Where its top-left corner goes.
         */
        void stamp(const Canvas &clip, Pixel at);

        /**
         * @brief Make every pixel of a rectangle transparent.
         * @param area The rectangle to clear, which must be inside.
         */
        void clearRegion(const Selection &area);

        Canvas sheet;
        TileGrid grid;
        Size area;
        CanvasView where;
        std::optional<SpriteGuides> outlines;

        Tool selected = Tool::Paint;
        Color paint;
        std::optional<std::size_t> swatch;
        bool showGrid = true;
        bool showGuides = true;
        std::optional<Pixel> under;
        // Only ever set through clampedTo().
        // So it is inside the sheet whenever it holds anything at all.
        // Which is what lets copy, cut and carry index it directly.
        std::optional<Selection> marked;
        std::optional<Gesture> gesture;
        std::optional<Canvas> clipboard;
        std::optional<StatusMessage> message;

        std::uint64_t changes = 0;
        std::uint64_t stepped = 0;
        std::uint32_t written = 0;
        std::uint32_t read = 0;
        std::uint64_t savedRevision = 0;
    };

} // namespace antwika::atlas_editor
