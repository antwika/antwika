#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/atlas_editor/Canvas.hpp"
#include "antwika/atlas_editor/CanvasView.hpp"
#include "antwika/atlas_editor/Pixel.hpp"
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
         * @brief Get which image pixel the pointer was last over.
         * @return The pixel, or nothing until an event has said where
         * the pointer is -- the folded default is the canvas's corner,
         * which is a pixel of the sheet and would read as a real hover.
         */
        [[nodiscard]] std::optional<Pixel> hovered() const noexcept;

        /**
         * @brief Get the last thing worth telling the artist.
         * @return The message, empty until something has happened.
         */
        [[nodiscard]] const std::string &status() const noexcept;

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
         * @param point Where the pointer was; one outside the image
         * changes nothing, since a drag off the edge of the sheet is
         * ordinary input rather than an error.
         */
        void applyAt(Point point) noexcept;

        /**
         * @brief Make the pixel under a canvas position transparent.
         *
         * What the right button does whichever tool is selected, so that
         * taking a mistake back never costs a trip to the toolbar.
         *
         * @param point Where the pointer was.
         */
        void eraseAt(Point point) noexcept;

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
         * @param message What to show.
         */
        void setStatus(std::string message);

    private:
        Canvas sheet;
        TileGrid grid;
        Size area;
        CanvasView where;

        Tool selected = Tool::Paint;
        Color paint;
        std::optional<std::size_t> swatch;
        bool showGrid = true;
        std::optional<Pixel> under;
        std::string message;

        std::uint64_t changes = 0;
        std::uint64_t stepped = 0;
        std::uint32_t written = 0;
        std::uint32_t read = 0;
        std::uint64_t savedRevision = 0;
    };

} // namespace antwika::atlas_editor
