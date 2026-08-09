#include "antwika/atlas_editor/EditorScene.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/atlas_editor/CanvasView.hpp"
#include "antwika/atlas_editor/Pixel.hpp"
#include "antwika/atlas_editor/Preview.hpp"
#include "antwika/atlas_editor/Selection.hpp"
#include "antwika/atlas_editor/Shapes.hpp"
#include "antwika/atlas_editor/SpriteGuides.hpp"
#include "antwika/atlas_editor/TileGrid.hpp"

namespace antwika::atlas_editor
{

    using antwika::gfx::Color;
    using antwika::gfx::Point;
    using antwika::gfx::Rect;

    namespace
    {
        constexpr Color kBackground{
            .red = 14, .green = 15, .blue = 18, .alpha = 255};

        constexpr Color kSheetBacking{
            .red = 40, .green = 42, .blue = 48, .alpha = 255};

        constexpr Color kSheetChecker{
            .red = 56, .green = 58, .blue = 64, .alpha = 255};

        constexpr std::uint32_t kCheckerSquare = 8;

        constexpr Color kUntinted{
            .red = 255, .green = 255, .blue = 255, .alpha = 255};

        constexpr Color kSlotLine{
            .red = 246, .green = 214, .blue = 96, .alpha = 200};

        constexpr Color kGuideLine{
            .red = 96, .green = 208, .blue = 232, .alpha = 190};

        constexpr Color kPivotMark{
            .red = 168, .green = 240, .blue = 255, .alpha = 235};

        constexpr std::int32_t kPivotArm = 2;

        constexpr Color kPixelLine{
            .red = 255, .green = 255, .blue = 255, .alpha = 40};

        constexpr std::uint32_t kPixelGridScale = 4;

        constexpr Color kSelectionLine{
            .red = 255, .green = 96, .blue = 200, .alpha = 235};

        constexpr Color kHover{
            .red = 255, .green = 255, .blue = 255, .alpha = 110};

        constexpr Color kHoverEdge{
            .red = 246, .green = 214, .blue = 96, .alpha = 255};

        constexpr Color kPaneEdge{
            .red = 96, .green = 104, .blue = 124, .alpha = 255};

        constexpr Color kPaneBacking{
            .red = 22, .green = 24, .blue = 30, .alpha = 255};

        constexpr Color kSlotMark{
            .red = 246, .green = 214, .blue = 96, .alpha = 200};

        constexpr std::uint32_t kPaneEdgeWidth = 1;

        constexpr Color kStrokePreview{
            .red = 255, .green = 255, .blue = 255, .alpha = 170};

        [[nodiscard]] std::int32_t rightOf(const Rect &rect) noexcept
        {
            return rect.origin.x
                   + static_cast<std::int32_t>(rect.size.width);
        }

        [[nodiscard]] std::int32_t bottomOf(const Rect &rect) noexcept
        {
            return rect.origin.y
                   + static_cast<std::int32_t>(rect.size.height);
        }

        [[nodiscard]] Point onCanvas(
            const Rect &area,
            const std::uint32_t scale,
            const std::int32_t x,
            const std::int32_t y) noexcept
        {
            return Point{
                .x = area.origin.x + x * static_cast<std::int32_t>(scale),
                .y = area.origin.y + y * static_cast<std::int32_t>(scale)};
        }

        [[nodiscard]] std::uint32_t squaresOver(
            const std::uint32_t span) noexcept
        {
            return (span + kCheckerSquare - 1) / kCheckerSquare;
        }

        [[nodiscard]] std::uint32_t squareEnd(
            const std::uint32_t at, const std::uint32_t span) noexcept
        {
            return std::min(at + kCheckerSquare, span);
        }

        void drawSheetBacking(
            IRenderer &renderer,
            const SceneSnapshot &snapshot,
            const Rect &area)
        {
            renderer.drawRect(area, kSheetBacking);

            const std::uint32_t scale = scaleOf(snapshot.view);
            const auto across = squaresOver(snapshot.image.width);
            const auto down = squaresOver(snapshot.image.height);

            for (std::uint32_t row = 0; row < down; ++row)
            {
                for (std::uint32_t column = row % 2; column < across;
                     column += 2)
                {
                    const auto left = column * kCheckerSquare;
                    const auto top = row * kCheckerSquare;

                    const Point corner = onCanvas(
                        area,
                        scale,
                        static_cast<std::int32_t>(left),
                        static_cast<std::int32_t>(top));

                    renderer.drawRect(
                        Rect{
                            .origin = corner,
                            .size =
                                {.width =
                                     (squareEnd(
                                          left, snapshot.image.width)
                                      - left)
                                     * scale,
                                 .height =
                                     (squareEnd(
                                          top, snapshot.image.height)
                                      - top)
                                     * scale}},
                        kSheetChecker);
                }
            }
        }

        void drawHover(
            IRenderer &renderer, const SceneSnapshot &snapshot)
        {
            const Rect cell =
                pixelRect(snapshot.view, *snapshot.hovered);

            renderer.drawRect(cell, kHover);

            if (!snapshot.pointerBorder)
            {
                return;
            }

            const Point corner = cell.origin;
            const Point far{
                .x = rightOf(cell), .y = bottomOf(cell)};

            renderer.drawLine(
                corner, Point{.x = far.x, .y = corner.y}, kHoverEdge);
            renderer.drawLine(
                Point{.x = far.x, .y = corner.y}, far, kHoverEdge);
            renderer.drawLine(
                far, Point{.x = corner.x, .y = far.y}, kHoverEdge);
            renderer.drawLine(
                Point{.x = corner.x, .y = far.y}, corner, kHoverEdge);
        }

        void drawSlotGrid(
            IRenderer &renderer,
            const SceneSnapshot &snapshot,
            const Rect &area)
        {
            const std::uint32_t scale = scaleOf(snapshot.view);
            const std::uint32_t columns =
                columnsIn(snapshot.tiles, snapshot.image);
            const std::uint32_t rows =
                rowsIn(snapshot.tiles, snapshot.image);

            for (std::uint32_t column = 0; column <= columns; ++column)
            {
                const auto x = area.origin.x
                               + static_cast<std::int32_t>(
                                   column * snapshot.tiles.width * scale);

                renderer.drawLine(
                    Point{.x = x, .y = area.origin.y},
                    Point{.x = x, .y = bottomOf(area)},
                    kSlotLine);
            }

            for (std::uint32_t row = 0; row <= rows; ++row)
            {
                const auto y = area.origin.y
                               + static_cast<std::int32_t>(
                                   row * snapshot.tiles.height * scale);

                renderer.drawLine(
                    Point{.x = area.origin.x, .y = y},
                    Point{.x = rightOf(area), .y = y},
                    kSlotLine);
            }
        }

        [[nodiscard]] std::int32_t rowHalfWidth(
            const std::int32_t row, const Size footprint) noexcept
        {
            const auto height =
                static_cast<std::int32_t>(footprint.height);
            const auto half =
                static_cast<std::int32_t>(footprint.width / 2);

            const std::int32_t fromTip = std::min(row, height - row);

            return fromTip * 2 < half ? fromTip * 2 + 1 : half;
        }

        void stepTo(std::vector<Point> &walked, const Point corner)
        {
            if (!walked.empty() && walked.back() == corner)
            {
                return;
            }

            walked.push_back(corner);
        }

        /**
         * @brief Walks the pixel edges around an isometric footprint.
         *
         * @param footprint The tile the guides sit on.
         * @return The corners of a closed staircase, each relative to
         *         the pivot corner.
         *
         * Ensures: consecutive corners differ along one axis only, so
         *          every leg lies on a pixel edge.
         */
        [[nodiscard]] std::vector<Point> diamondOutline(const Size footprint)
        {
            const auto height =
                static_cast<std::int32_t>(footprint.height);

            std::vector<Point> walked;

            stepTo(
                walked,
                Point{.x = rowHalfWidth(0, footprint), .y = 0});

            for (std::int32_t row = 0; row <= height; ++row)
            {
                const std::int32_t edge = -(row + 1);
                const std::int32_t above = row == height ? row : row + 1;

                stepTo(
                    walked,
                    Point{
                        .x = rowHalfWidth(row, footprint), .y = edge});
                stepTo(
                    walked,
                    Point{
                        .x = rowHalfWidth(above, footprint), .y = edge});
            }

            stepTo(
                walked,
                Point{
                    .x = -rowHalfWidth(height, footprint),
                    .y = -(height + 1)});

            for (std::int32_t row = height; row >= 0; --row)
            {
                const std::int32_t edge = -row;
                const std::int32_t below = row == 0 ? row : row - 1;

                stepTo(
                    walked,
                    Point{
                        .x = -rowHalfWidth(row, footprint), .y = edge});
                stepTo(
                    walked,
                    Point{
                        .x = -rowHalfWidth(below, footprint), .y = edge});
            }

            const Point closed = walked.front();
            stepTo(walked, closed);

            return walked;
        } // GCOVR_EXCL_LINE

        void drawOutlineInSlot(
            IRenderer &renderer,
            const Rect &area,
            const std::uint32_t scale,
            const std::vector<Point> &outline,
            const Point pivot)
        {
            for (std::size_t at = 0; at + 1 < outline.size(); ++at)
            {
                renderer.drawLine(
                    onCanvas(
                        area,
                        scale,
                        pivot.x + outline[at].x,
                        pivot.y + outline[at].y),
                    onCanvas(
                        area,
                        scale,
                        pivot.x + outline[at + 1].x,
                        pivot.y + outline[at + 1].y),
                    kGuideLine);
            }
        }

        void drawPivotInSlot(
            IRenderer &renderer,
            const Rect &area,
            const std::uint32_t scale,
            const Point pivot)
        {
            renderer.drawLine(
                onCanvas(area, scale, pivot.x - kPivotArm, pivot.y),
                onCanvas(area, scale, pivot.x + kPivotArm, pivot.y),
                kPivotMark);
            renderer.drawLine(
                onCanvas(area, scale, pivot.x, pivot.y - kPivotArm),
                onCanvas(area, scale, pivot.x, pivot.y + kPivotArm),
                kPivotMark);
        }

        void drawSelection(
            IRenderer &renderer,
            const SceneSnapshot &snapshot,
            const Rect &area)
        {
            const std::uint32_t scale = scaleOf(snapshot.view);
            const Selection &marked = *snapshot.selection;

            const Point corner = onCanvas(
                area, scale, marked.origin.x, marked.origin.y);
            const Point far = onCanvas(
                area,
                scale,
                marked.origin.x
                    + static_cast<std::int32_t>(marked.size.width),
                marked.origin.y
                    + static_cast<std::int32_t>(marked.size.height));

            renderer.drawLine(
                corner, Point{.x = far.x, .y = corner.y}, kSelectionLine);
            renderer.drawLine(
                Point{.x = far.x, .y = corner.y}, far, kSelectionLine);
            renderer.drawLine(
                far, Point{.x = corner.x, .y = far.y}, kSelectionLine);
            renderer.drawLine(
                Point{.x = corner.x, .y = far.y}, corner, kSelectionLine);
        }

        void drawPixelGrid(
            IRenderer &renderer,
            const SceneSnapshot &snapshot,
            const Rect &area)
        {
            const std::uint32_t scale = scaleOf(snapshot.view);

            if (scale < kPixelGridScale)
            {
                return;
            }

            for (std::uint32_t column = 0;
                 column <= snapshot.image.width;
                 ++column)
            {
                const auto x = area.origin.x
                    + static_cast<std::int32_t>(column * scale);

                renderer.drawLine(
                    Point{.x = x, .y = area.origin.y},
                    Point{.x = x, .y = bottomOf(area)},
                    kPixelLine);
            }

            for (std::uint32_t row = 0; row <= snapshot.image.height;
                 ++row)
            {
                const auto y = area.origin.y
                    + static_cast<std::int32_t>(row * scale);

                renderer.drawLine(
                    Point{.x = area.origin.x, .y = y},
                    Point{.x = rightOf(area), .y = y},
                    kPixelLine);
            }
        }

        void drawStroke(
            IRenderer &renderer, const SceneSnapshot &snapshot)
        {
            for (const Pixel pixel : shapePixels(
                     snapshot.tool,
                     snapshot.stroke->from,
                     snapshot.stroke->to,
                     snapshot.image))
            {
                renderer.drawRect(
                    pixelRect(snapshot.view, pixel), kStrokePreview);
            }
        }

        [[nodiscard]] Point pivotOfSlot(
            const SceneSnapshot &snapshot,
            const Point anchor,
            const std::uint32_t column,
            const std::uint32_t row) noexcept
        {
            return Point{
                .x = static_cast<std::int32_t>(
                         column * snapshot.tiles.width)
                     + anchor.x,
                .y = static_cast<std::int32_t>(
                         row * snapshot.tiles.height)
                     + anchor.y};
        }

        void drawSpriteGuides(
            IRenderer &renderer,
            const SceneSnapshot &snapshot,
            const Rect &area)
        {
            const std::uint32_t scale = scaleOf(snapshot.view);
            const std::uint32_t columns =
                columnsIn(snapshot.tiles, snapshot.image);
            const std::uint32_t rows =
                rowsIn(snapshot.tiles, snapshot.image);

            const auto outline = snapshot.guides.has_value()
                                     ? diamondOutline(
                                           snapshot.guides->footprint)
                                     : std::vector<Point>{};

            for (std::uint32_t row = 0; row < rows; ++row)
            {
                for (std::uint32_t column = 0; column < columns; ++column)
                {
                    if (snapshot.guides.has_value())
                    {
                        drawOutlineInSlot(
                            renderer,
                            area,
                            scale,
                            outline,
                            pivotOfSlot(
                                snapshot,
                                snapshot.guides->pivot,
                                column,
                                row));
                    }

                    if (snapshot.pivot.has_value())
                    {
                        drawPivotInSlot(
                            renderer,
                            area,
                            scale,
                            pivotOfSlot(
                                snapshot, *snapshot.pivot, column, row));
                    }
                }
            }
        }
    }

    namespace
    {
        void drawPaneEdge(IRenderer &renderer, const Rect &pane)
        {
            renderer.drawRect(
                Rect{
                    .origin = pane.origin,
                    .size = {
                        .width = pane.size.width,
                        .height = kPaneEdgeWidth}},
                kPaneEdge);

            renderer.drawRect(
                Rect{
                    .origin = {.x = pane.origin.x,
                               .y = bottomOf(pane)
                                    - static_cast<std::int32_t>(
                                        kPaneEdgeWidth)},
                    .size = {
                        .width = pane.size.width,
                        .height = kPaneEdgeWidth}},
                kPaneEdge);

            renderer.drawRect(
                Rect{
                    .origin = pane.origin,
                    .size = {
                        .width = kPaneEdgeWidth,
                        .height = pane.size.height}},
                kPaneEdge);

            renderer.drawRect(
                Rect{
                    .origin = {.x = rightOf(pane)
                                    - static_cast<std::int32_t>(
                                        kPaneEdgeWidth),
                               .y = pane.origin.y},
                    .size = {
                        .width = kPaneEdgeWidth,
                        .height = pane.size.height}},
                kPaneEdge);
        }

        [[nodiscard]] bool holdsWhole(
            const Rect &outer, const Rect &inner) noexcept
        {
            return inner.origin.x >= outer.origin.x
                   && inner.origin.y >= outer.origin.y
                   && rightOf(inner) <= rightOf(outer)
                   && bottomOf(inner) <= bottomOf(outer);
        }

        void drawSlotMark(
            IRenderer &renderer,
            const PreviewShot &shot,
            const Rect &slot)
        {
            const auto scale = scaleOf(shot.view);

            const Rect marked{
                .origin = pixelRect(
                              shot.view,
                              Pixel{.x = slot.origin.x, .y = slot.origin.y})
                              .origin,
                .size = {
                    .width = slot.size.width * scale,
                    .height = slot.size.height * scale}};

            if (!holdsWhole(shot.pane, marked))
            {
                return;
            }

            const Point corner = marked.origin;
            const Point far{
                .x = rightOf(marked), .y = bottomOf(marked)};

            renderer.drawLine(
                corner, Point{.x = far.x, .y = corner.y}, kSlotMark);
            renderer.drawLine(
                Point{.x = far.x, .y = corner.y}, far, kSlotMark);
            renderer.drawLine(
                far, Point{.x = corner.x, .y = far.y}, kSlotMark);
            renderer.drawLine(
                Point{.x = corner.x, .y = far.y}, corner, kSlotMark);
        }

        void drawPreview(
            IRenderer &renderer,
            const PreviewShot &shot,
            const Size image,
            const ITexture *sheet)
        {
            renderer.drawRect(shot.pane, kPaneBacking);

            const auto blit = blitFor(shot.view, shot.pane, image);

            if (blit.has_value() && sheet != nullptr)
            {
                renderer.drawTexture(
                    *sheet, blit->source, blit->destination, kUntinted);
            }

            if (shot.slot.has_value())
            {
                drawSlotMark(renderer, shot, *shot.slot);
            }

            drawPaneEdge(renderer, shot.pane);
        }
    }

    void EditorScene::draw(
        IRenderer &renderer,
        const SceneSnapshot &snapshot,
        const ITexture *image) const
    {
        renderer.clear(kBackground);

        const Rect area = imageRect(snapshot.view, snapshot.image);

        drawSheetBacking(renderer, snapshot, area);

        if (image != nullptr)
        {
            renderer.drawTexture(
                *image,
                Rect{.origin = {}, .size = snapshot.image},
                area,
                kUntinted);
        }

        if (snapshot.pixelGridVisible)
        {
            drawPixelGrid(renderer, snapshot, area);
        }

        if (snapshot.gridVisible)
        {
            drawSlotGrid(renderer, snapshot, area);
        }

        if (snapshot.guides.has_value() || snapshot.pivot.has_value())
        {
            drawSpriteGuides(renderer, snapshot, area);
        }

        if (snapshot.selection.has_value())
        {
            drawSelection(renderer, snapshot, area);
        }

        if (snapshot.stroke.has_value())
        {
            drawStroke(renderer, snapshot);
        }

        if (snapshot.hovered.has_value())
        {
            drawHover(renderer, snapshot);
        }

        if (snapshot.preview.has_value())
        {
            drawPreview(
                renderer, *snapshot.preview, snapshot.image, image);
        }
    }

}
