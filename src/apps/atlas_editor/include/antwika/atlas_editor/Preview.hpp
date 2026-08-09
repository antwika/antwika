#pragma once

#include <cstdint>
#include <optional>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/ui/SplitSpec.hpp>

#include "antwika/atlas_editor/CanvasView.hpp"
#include "antwika/atlas_editor/TileGrid.hpp"

namespace antwika::atlas_editor
{

    using antwika::gfx::Rect;
    using antwika::gfx::Size;

    inline constexpr std::uint32_t kPreviewRatio =
        antwika::ui::kWholeSplit * 2 / 3;

    inline constexpr std::uint32_t kPaneMinimum = 96;

    struct PreviewPane final
    {
        bool open = false;

        bool autoFocus = true;

        std::uint32_t ratio = kPreviewRatio;

        bool dragging = false;

        CanvasView view{};

        std::optional<std::uint32_t> focused{};

        [[nodiscard]] bool operator==(const PreviewPane &other) const =
            default;
    };

    struct Blit final
    {
        Rect source{};

        Rect destination{};

        [[nodiscard]] bool operator==(const Blit &other) const = default;
    };

    [[nodiscard]] bool paneHolds(
        Rect pane, antwika::gfx::Point at) noexcept;

    /**
     * @brief Works out which part of a sheet a pane is looking at.
     *
     * @param view Where the pane is panned and how far it is zoomed.
     * @param pane The pane's rectangle on the canvas.
     * @param image The sheet's size in pixels.
     * @return The sheet pixels under the pane and where they land in
     *         it, or nothing if the pane is looking off the sheet.
     */
    [[nodiscard]] std::optional<Blit> blitFor(
        CanvasView view, Rect pane, Size image) noexcept;

    /**
     * @brief Frames a slot as large as a pane can show it whole.
     *
     * @param pane The pane's rectangle on the canvas.
     * @param slot The slot's rectangle in sheet pixels.
     * @return A view that centres the slot in the pane.
     */
    [[nodiscard]] CanvasView fittedView(Rect pane, Rect slot) noexcept;

    /**
     * @brief Frames the slot a pixel falls in.
     *
     * @param pane The pane's rectangle on the canvas.
     * @param tiles The grid the sheet is cut into.
     * @param image The sheet's size in pixels.
     * @param slot The slot to frame, counted across then down.
     * @return A view centred on that slot, or nothing if the grid has
     *         no such slot.
     */
    [[nodiscard]] std::optional<CanvasView> viewOfSlot(
        Rect pane, TileGrid tiles, Size image, std::uint32_t slot) noexcept;

}
