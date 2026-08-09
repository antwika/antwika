#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>

#include <antwika/app/preview/DrawnPreview.hpp>
#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/atlas_editor/Canvas.hpp"
#include "antwika/atlas_editor/EditorScene.hpp"
#include "antwika/atlas_editor/EditorState.hpp"
#include "antwika/atlas_editor/Pixel.hpp"
#include "antwika/atlas_editor/SceneSnapshot.hpp"
#include "antwika/atlas_editor/TileGrid.hpp"

namespace
{
    using antwika::atlas_editor::Canvas;
    using antwika::atlas_editor::EditorScene;
    using antwika::atlas_editor::EditorState;
    using antwika::atlas_editor::Pixel;
    using antwika::atlas_editor::snapshotOf;
    using antwika::atlas_editor::TileGrid;
    using antwika::gfx::Color;
    using antwika::app::preview::drawnPreview;
    using antwika::gfx::IRenderer;
    using antwika::gfx::Size;

    constexpr Size kCanvas{.width = 800, .height = 600};

    constexpr Size kSheet{.width = 32, .height = 32};

    [[nodiscard]] antwika::gfx::Bitmap drawn()
    {
        antwika::gfx::Bitmap sheet;
        sheet.size = kSheet;
        sheet.pixels.assign(
            static_cast<std::size_t>(kSheet.width) * kSheet.height
                * antwika::gfx::kBytesPerPixel,
            0);

        for (std::uint32_t y = 4; y < 28; ++y)
        {
            for (std::uint32_t x = 4; x < 28; ++x)
            {
                if ((x / 4 + y / 4) % 2 == 0)
                {
                    continue;
                }

                const auto at = (y * kSheet.width + x)
                    * antwika::gfx::kBytesPerPixel;

                sheet.pixels[at] = static_cast<std::uint8_t>(8 * x);
                sheet.pixels[at + 1] = static_cast<std::uint8_t>(8 * y);
                sheet.pixels[at + 2] = 160;
                sheet.pixels[at + 3] = 255;
            }
        }

        return sheet;
    }

    [[nodiscard]] EditorState painted()
    {
        EditorState state{
            Canvas(drawn()),
            TileGrid{.width = 16, .height = 16},
            kCanvas};

        state.toggleGrid();

        for (std::size_t step = 0; step < 4; ++step)
        {
            state.zoomIn(
                antwika::gfx::Point{
                    .x = static_cast<std::int32_t>(kCanvas.width / 2),
                    .y = static_cast<std::int32_t>(kCanvas.height / 2)});
        }

        return state;
    }
}

TEST(SheetPreviewTest, Draw_WritesASheetUnderTheGrid)
{
    EXPECT_FALSE(
        drawnPreview(
            {.name = "atlas-editor",
             .title = "Antwika Atlas Editor",
             .canvas = kCanvas},
            [](IRenderer &renderer)
            {
                const auto state = painted();
                const auto sheet =
                    renderer.createTexture(state.image().bitmap());

                const EditorScene scene;
                scene.draw(renderer, snapshotOf(state), sheet.get());
            })
            .empty());
}
