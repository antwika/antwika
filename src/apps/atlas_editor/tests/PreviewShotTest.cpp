#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/app/preview/DrawnPreview.hpp>
#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/ui/Pointer.hpp>

#include "antwika/atlas_editor/Canvas.hpp"
#include "antwika/atlas_editor/EditorScene.hpp"
#include "antwika/atlas_editor/EditorState.hpp"
#include "antwika/atlas_editor/EditorUi.hpp"
#include "antwika/atlas_editor/Messages.hpp"
#include "antwika/atlas_editor/SceneSnapshot.hpp"
#include "antwika/atlas_editor/TileGrid.hpp"

namespace
{
    using antwika::app::preview::drawnPreview;
    using antwika::atlas_editor::Canvas;
    using antwika::atlas_editor::EditorScene;
    using antwika::atlas_editor::EditorState;
    using antwika::atlas_editor::snapshotOf;
    using antwika::atlas_editor::TileGrid;
    using antwika::gfx::IRenderer;
    using antwika::gfx::Rect;
    using antwika::gfx::Size;

    constexpr Size kCanvas{.width = 800, .height = 600};
    constexpr Size kSheet{.width = 64, .height = 64};
}

TEST(PreviewShotTest, Draw_WritesTheSheetBesideItsPreview)
{
    antwika::gfx::Bitmap bitmap;
    bitmap.size = kSheet;
    bitmap.pixels.assign(
        static_cast<std::size_t>(kSheet.width) * kSheet.height
            * antwika::gfx::kBytesPerPixel, 0);

    for (std::uint32_t y = 0; y < kSheet.height; ++y)
    {
        for (std::uint32_t x = 0; x < kSheet.width; ++x)
        {
            const auto at =
                (y * kSheet.width + x) * antwika::gfx::kBytesPerPixel;
            bitmap.pixels[at] = static_cast<std::uint8_t>(x * 4);
            bitmap.pixels[at + 1] = static_cast<std::uint8_t>(y * 4);
            bitmap.pixels[at + 2] = 200;
            bitmap.pixels[at + 3] = 255;
        }
    }

    EditorState state(
        Canvas(bitmap), TileGrid{.width = 16, .height = 16}, kCanvas);

    state.togglePreview();
    state.noteTouched(antwika::atlas_editor::Pixel{.x = 20, .y = 20});

    const Rect pane =
        antwika::atlas_editor::describeEditor(
            state, antwika::ui::Pointer{},
            antwika::atlas_editor::Translator{
                antwika::i18n::kDefaultLocale})
            .rects.find(antwika::atlas_editor::widgets::kPreviewPane)
            .value_or(Rect{});

    state.focusPreviewOn(antwika::atlas_editor::PreviewPane{
        .view = *antwika::atlas_editor::viewOfSlot(
            pane,
            state.tiles(),
            state.image().size(),
            *state.preview().focused)});

    EXPECT_FALSE(
        drawnPreview(
            {.name = "atlas-editor-preview",
             .title = "Antwika Atlas Editor",
             .canvas = kCanvas},
            [&](IRenderer &renderer)
            {
                const auto sheet =
                    renderer.createTexture(state.image().bitmap());

                const EditorScene scene;
                scene.draw(
                    renderer, snapshotOf(state, pane), sheet.get());
            })
            .empty());
}
