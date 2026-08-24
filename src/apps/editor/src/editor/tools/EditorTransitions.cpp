#include <antwika/editor/ui/EditorLook.hpp>
#include <antwika/geometry/Grid.hpp>
#include <antwika/tilemap/Tilemap.hpp>

#include "antwika/editor/Editor.hpp"

namespace antwika::editor
{

    bool Editor::pickedTransition(const tilemap::Tile tile)
    {
        if (!transition.fromTile.has_value())
        {
            return false;
        }

        if (transitionOf(document.map.transitions, tile) != nullptr
            || tile.atlas != transition.fromTile->atlas)
        {
            showStatus(
                "a material is a plain tile of the same "
                "atlas",
                false,
                360);

            return true;
        }

        if (!transition.toTile.has_value())
        {
            if (tile == *transition.fromTile)
            {
                showStatus(
                    "the other material is another tile",
                    false,
                    360);

                return true;
            }

            transition.toTile = tile;
            showStatus(
                "pick the mask tile", false, 360);

            return true;
        }

        const auto slot = tile::getFirstUnusedTile(document.map.tilemap,
            tile.atlas);

        if (!slot.has_value())
        {
            showStatus(
                "no slot of this atlas is free", false, 360);
            transition.fromTile.reset();
            transition.toTile.reset();

            return true;
        }

        pushUndo();
        document.map.transitions.push_back(
            tile::TransitionTile{
                .fromTile = *transition.fromTile,
                .toTile = *transition.toTile,
                .maskTile = tile,
                .outputTile = *slot});
        transition.fromTile.reset();
        transition.toTile.reset();

        for (std::uint32_t row = 0;
             row < document.map.tilemap.rows;
             ++row)
        {
            for (std::uint32_t column = 0;
                 column < document.map.tilemap.columns;
                 ++column)
            {
                const auto place = geometry::GridCell{
                    .column = column, .row = row};

                if (document.map.tilemap
                        .getEntryAt(place.column, place.row)
                        .has_value())
                {
                    continue;
                }

                tilemap::putTile(document.map.tilemap, place, *slot);
                row = document.map.tilemap.rows;

                break;
            }
        }

        atlasSheets.touch();
        rebuildWorld();
        showStatus(
            "woven into slot "
            + std::to_string(slot->index));

        return true;
    }

    bool Editor::transitionWidgets(
        const ui::Interactions &interactions)
    {
        auto consumedKey = false;

        if (interactions.activatedWidget
                == tile::kTransitionAddWidget
            && stroke.selectedTile.has_value()
            && document.map.transitions.size() < tile::kMaxTransitions)
        {
            transition.fromTile = stroke.selectedTile;
            transition.toTile.reset();
            showStatus(
                "pick the other material", false, 360);
            consumedKey = true;
        }

        if (interactions.activatedWidget
                == tile::kRemoveTransitionWidget
            && transition.chosenIndex.has_value()
            && *transition.chosenIndex < document.map.transitions.size())
        {
            pushUndo();
            document.map.transitions.erase(
                std::next(
                    document.map.transitions.begin(),
                    static_cast<std::ptrdiff_t>(
                        *transition.chosenIndex)));
            transition.chosenIndex.reset();
            atlasSheets.touch();
            rebuildWorld();
            consumedKey = true;
        }

        for (std::size_t index = 0;
             index < document.map.transitions.size()
             && index < tile::kMaxTransitions;
             ++index)
        {
            if (interactions.activatedWidget
                != tile::getTransitionRowWidget(index))
            {
                continue;
            }

            transition.chosenIndex =
                transition.chosenIndex == index
                                  ? std::optional<std::size_t>{}
                                  : std::optional{index};
            consumedKey = true;
        }

        return consumedKey;
    }

    void Editor::layoutTransitionRail(ui::Context &context)
    {
        const auto transitionsPanel = context.column(
            antwika::ui::ContainerSpec{
                .widthSizing = antwika::ui::kGrowSizing,
                .backgroundColor = kPanelColor,
                .padding = kPanelPadding});

        panelTitle(context, "Transitions");

        for (std::size_t index = 0;
             index < document.map.transitions.size();
             ++index)
        {
            const auto &rowTransition = document.map.transitions.at(index);

            if (rowTransition.fromTile != *stroke.selectedTile
                && rowTransition.toTile != *stroke.selectedTile
                && rowTransition.maskTile != *stroke.selectedTile
                && rowTransition.outputTile != *stroke.selectedTile)
            {
                continue;
            }

            context.button(
                "slot "
                    + std::to_string(
                        rowTransition.outputTile.index),
                antwika::ui::ButtonSpec{
                    .widgetId = tile::getTransitionRowWidget(
                        index),
                    .widthSizing = antwika::ui::kGrowSizing,
                    .fillColor = transition.chosenIndex == index
                               ? kSelectionAccentColor
                               : kGridLineColor});
        }

        if (transition.chosenIndex.has_value())
        {
            context.button(
                "x",
                antwika::ui::ButtonSpec{
                    .widgetId = tile::
                        kRemoveTransitionWidget});
        }

        if (document.map.transitions.size() < tile::kMaxTransitions)
        {
            context.button(
                transition.fromTile.has_value()
                    ? (transition.toTile.has_value()
                           ? "mask?"
                           : "other?")
                    : "new transition",
                antwika::ui::ButtonSpec{
                    .widgetId = tile::kTransitionAddWidget,
                    .widthSizing = antwika::ui::kGrowSizing});
        }
    }

}
