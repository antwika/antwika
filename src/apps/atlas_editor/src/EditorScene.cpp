#include "antwika/atlas_editor/EditorScene.hpp"

#include <cstdint>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>

#include "antwika/atlas_editor/CanvasView.hpp"
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
            .red = 24, .green = 26, .blue = 30, .alpha = 255};

        // What shows through a transparent pixel.
        // Dark, and not black.
        // A pixel painted black still has to read as somebody's.
        constexpr Color kSheetBacking{
            .red = 58, .green = 60, .blue = 66, .alpha = 255};

        constexpr Color kUntinted{
            .red = 255, .green = 255, .blue = 255, .alpha = 255};

        constexpr Color kSlotLine{
            .red = 246, .green = 214, .blue = 96, .alpha = 200};

        // Told apart from the slot grid by hue rather than by weight.
        // The two cross each other and are read together.
        // A second yellow would be one grid drawn at two spacings.
        constexpr Color kGuideLine{
            .red = 96, .green = 208, .blue = 232, .alpha = 190};

        // The pivot reads brighter than the diamond it belongs to.
        // It is the one point a blit is anchored by.
        constexpr Color kPivotMark{
            .red = 168, .green = 240, .blue = 255, .alpha = 235};

        // How far each arm of the pivot cross reaches, in image pixels.
        // In image pixels rather than canvas ones, so it grows with art.
        // A fixed mark would swamp the diamond at the widest zoom.
        // And vanish into it at the closest.
        constexpr std::int32_t kPivotArm = 2;

        constexpr Color kHover{
            .red = 255, .green = 255, .blue = 255, .alpha = 110};

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

            // One line per boundary, both ends included.
            // A sheet of eight columns is drawn with nine verticals.
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

        // A point of the sheet, in canvas pixels.
        // Lattice coordinates rather than pixel ones.
        // The slot grid's own lines are drawn on that same lattice.
        // A diamond's corners sit between pixels rather than on one.
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

        // One slot's footprint diamond and the pivot it stands on.
        // Drawn per slot rather than once.
        // Which slot a sprite is in is what an artist places it against.
        void drawGuidesInSlot(
            IRenderer &renderer,
            const Rect &area,
            const std::uint32_t scale,
            const SpriteGuides &guides,
            const Point &origin)
        {
            const std::int32_t pivotX = origin.x + guides.pivot.x;
            const std::int32_t pivotY = origin.y + guides.pivot.y;

            const auto height =
                static_cast<std::int32_t>(guides.footprint.height);
            const auto half =
                static_cast<std::int32_t>(guides.footprint.width / 2);

            const Point bottom = onCanvas(area, scale, pivotX, pivotY);
            const Point top =
                onCanvas(area, scale, pivotX, pivotY - height);
            const Point left = onCanvas(
                area, scale, pivotX - half, pivotY - height / 2);
            const Point right = onCanvas(
                area, scale, pivotX + half, pivotY - height / 2);

            renderer.drawLine(top, right, kGuideLine);
            renderer.drawLine(right, bottom, kGuideLine);
            renderer.drawLine(bottom, left, kGuideLine);
            renderer.drawLine(left, top, kGuideLine);

            renderer.drawLine(
                onCanvas(area, scale, pivotX - kPivotArm, pivotY),
                onCanvas(area, scale, pivotX + kPivotArm, pivotY),
                kPivotMark);
            renderer.drawLine(
                onCanvas(area, scale, pivotX, pivotY - kPivotArm),
                onCanvas(area, scale, pivotX, pivotY + kPivotArm),
                kPivotMark);
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

            // Whole slots only, on columnsIn()'s terms.
            // A strip too narrow for a whole slot belongs to no slot.
            // So there is no sprite there to be shaped.
            for (std::uint32_t row = 0; row < rows; ++row)
            {
                for (std::uint32_t column = 0; column < columns; ++column)
                {
                    drawGuidesInSlot(
                        renderer,
                        area,
                        scale,
                        *snapshot.guides,
                        Point{
                            .x = static_cast<std::int32_t>(
                                column * snapshot.tiles.width),
                            .y = static_cast<std::int32_t>(
                                row * snapshot.tiles.height)});
                }
            }
        }
    } // namespace

    void EditorScene::draw(
        IRenderer &renderer,
        const SceneSnapshot &snapshot,
        const ITexture *image) const
    {
        renderer.clear(kBackground);

        const Rect area = imageRect(snapshot.view, snapshot.image);

        renderer.drawRect(area, kSheetBacking);

        if (image != nullptr)
        {
            renderer.drawTexture(
                *image,
                Rect{.origin = {}, .size = snapshot.image},
                area,
                kUntinted);
        }

        if (snapshot.gridVisible)
        {
            drawSlotGrid(renderer, snapshot, area);
        }

        // After the slot grid and before the hover.
        // A diamond is inside a slot, and the pointer is over both.
        if (snapshot.guides.has_value())
        {
            drawSpriteGuides(renderer, snapshot, area);
        }

        if (snapshot.hovered.has_value())
        {
            renderer.drawRect(
                pixelRect(snapshot.view, *snapshot.hovered), kHover);
        }
    }

} // namespace antwika::atlas_editor
