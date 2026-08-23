#include <antwika/decor/Decor.hpp>
#include <antwika/editor/ui/EditorLook.hpp>
#include <antwika/tilemap/Tilemap.hpp>

#include "antwika/editor/Editor.hpp"

namespace antwika::editor
{

    bool Editor::shouldAdvanceTileAnimation() const
    {
        return isAnyTileAnimated(document.map.flipAnimations)
               && tick % decor::kDecorPaceTick == 0
               && !strokeActive;
    }

    bool Editor::flipWidgets(
        const ui::Interactions &interactions)
    {
        auto consumedKey = false;

        if (interactions.activatedWidget
                == decor::kToggleAnimationWidget
            && selectedTile.has_value())
        {
            pushUndo();
            document.map.flipAnimations =
                getWithAnimationToggled(document.map.flipAnimations,
                    *selectedTile);
            assignMode.flipFramePicked = 0;
            assignMode.flipFrameAssigning = false;
            atlasSheets.touch();
            consumedKey = true;
        }

        for (std::size_t frame = 0;
             frame < decor::kMaxDecorFrames;
             ++frame)
        {
            if (interactions.activatedWidget
                != decor::getFlipFrameWidget(frame))
            {
                continue;
            }

            clearAssignModes();
            assignMode.flipFramePicked = frame;
            assignMode.flipFrameAssigning = frame > 0;
            consumedKey = true;
        }

        if (interactions.activatedWidget
                == decor::kAddFrameWidget
            && selectedTile.has_value())
        {
            pushUndo();
            document.map.flipAnimations =
                getWithAnimationFrameAdded(document.map.flipAnimations,
                    *selectedTile);

            const auto *animation =
                animationOf(document.map.flipAnimations, *selectedTile);

            if (animation != nullptr && !animation->frameTiles.empty())
            {
                assignMode.flipFramePicked =
                    animation->frameTiles.size() - 1;
                assignMode.flipFrameAssigning =
                    assignMode.flipFramePicked > 0;
            }

            atlasSheets.touch();
            consumedKey = true;
        }

        return consumedKey;
    }

    void Editor::layoutFlipRail(ui::Context &context)
    {
        if (activeView != map::View::Atlases || !selectedTile.has_value()
            || isDecorLayer())
        {
            return;
        }

        const auto *animation =
            animationOf(document.map.flipAnimations, *selectedTile);
        const auto animationPanel = context.column(
            antwika::ui::ContainerSpec{
                .widthSizing = antwika::ui::kGrowSizing,
                .backgroundColor = kPanelColor,
                .padding = kPanelPadding});

        panelTitle(context, "Motion");
        context.checkbox(
            "animated",
            antwika::ui::CheckboxSpec{
                .widgetId = decor::kToggleAnimationWidget,
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
                        .widgetId = decor::getFlipFrameWidget(
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
                        .widgetId = decor::
                            kAddFrameWidget});
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
