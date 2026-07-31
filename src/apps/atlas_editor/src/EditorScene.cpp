#include "antwika/atlas_editor/EditorScene.hpp"

#include <cstdint>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>

#include "antwika/atlas_editor/CanvasView.hpp"
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

        if (snapshot.hovered.has_value())
        {
            renderer.drawRect(
                pixelRect(snapshot.view, *snapshot.hovered), kHover);
        }
    }

} // namespace antwika::atlas_editor
