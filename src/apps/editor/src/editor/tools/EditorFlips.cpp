#include <antwika/decor/Decor.hpp>
#include <antwika/editor/ui/EditorLook.hpp>
#include <antwika/tilemap/Tilemap.hpp>

#include "antwika/editor/Editor.hpp"

#include "antwika/editor/ui/WidgetIds.hpp"

namespace antwika::editor
{

    bool Editor::shouldAdvanceTileAnimation() const
    {
        return isAnyTileAnimated(document.map.flipAnimations)
               && tick % decor::kDecorPaceTick == 0
               && !stroke.active;
    }

    void Editor::layoutFlipRail(ui::Context &context)
    {
        const auto *animation =
            animationOf(document.map.flipAnimations, *stroke.selectedTile);
        const auto animationPanel = context.column(
            antwika::ui::ContainerSpec{
                .widthSizing = antwika::ui::kGrowSizing,
                .backgroundColor = kPanelColor,
                .padding = kPanelPadding});

        panelTitle(context, "Motion");
        context.checkbox(
            "animated",
            antwika::ui::CheckboxSpec{
                .widgetId = kToggleAnimationWidget,
                .checked = animation != nullptr});

        if (animation == nullptr)
        {
            return;
        }

        {
            const auto frames = context.row(
                antwika::ui::ContainerSpec{
                    .widthSizing = antwika::ui::kGrowSizing});

            for (std::size_t frame = 0;
                 frame < animation->frameTiles.size();
                 ++frame)
            {
                context.button(
                    std::to_string(frame + 1),
                    antwika::ui::ButtonSpec{
                        .widgetId = getFlipFrameWidget(
                            frame),
                        .fillColor = frame == assignMode.flipFramePicked
                                   ? kSelectionAccentColor
                                   : kGridLineColor});
            }

            if (animation->frameTiles.size()
                < decor::kMaxDecorFrames)
            {
                context.button(
                    "+",
                    antwika::ui::ButtonSpec{
                        .widgetId = kAddFrameWidget});
            }
        }

        const auto walking = animationFrameAt(*animation, tick);
        const auto shownFrom = tilemap::getTileSource(walking);
        const antwika::gfx::Rect walkCutRect{
            .originPoint =
                {.x = static_cast<std::int32_t>(
                     shownFrom.originPoint.x),
                 .y = static_cast<std::int32_t>(
                     shownFrom.originPoint.y)},
            .size = tilemap::tileSizeOf(walking.atlas)};

        context.image(
            antwika::ui::Icon{
                .sheetTexture = walking.atlas == tilemap::Atlas::Wall
                              ? atlasSheets.getTexture(tilemap::Atlas::Wall)
                              : atlasSheets.getTexture(tilemap::Atlas::Floor),
                .sourceRect = walkCutRect,
                .scale = kUiScale},
            kWhiteColor);
    }

}
