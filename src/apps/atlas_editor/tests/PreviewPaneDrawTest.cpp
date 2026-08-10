#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>

#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockTexture.hpp>
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
    using antwika::atlas_editor::Canvas;
    using antwika::atlas_editor::EditorScene;
    using antwika::atlas_editor::EditorState;
    using antwika::atlas_editor::snapshotOf;
    using antwika::atlas_editor::TileGrid;
    using antwika::gfx::Bitmap;
    using antwika::gfx::Rect;
    using antwika::gfx::Size;
    using antwika::gfx::mocks::MockRenderer;
    using antwika::gfx::mocks::MockTexture;
    using ::testing::_;
    using ::testing::AtLeast;
    using ::testing::NiceMock;

    constexpr Size kCanvas{.width = 800, .height = 600};
    constexpr Size kSheet{.width = 64, .height = 64};
}

TEST(PreviewPaneDrawTest, Draw_DrawsTheSheetBesideItsPreview)
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

    NiceMock<MockRenderer> renderer;

    ON_CALL(renderer, createTexture(_))
        .WillByDefault(
            [](const Bitmap &)
            { return std::make_unique<NiceMock<MockTexture>>(); });

    EXPECT_CALL(renderer, drawRect(_, _)).Times(AtLeast(1));

    const auto sheet = renderer.createTexture(state.image().bitmap());

    const EditorScene scene;

    scene.draw(renderer, snapshotOf(state, pane), sheet.get());
}
