#include <antwika/decor/Decor.hpp>
#include <antwika/tilemap/Tilemap.hpp>

#include "antwika/editor/Editor.hpp"

namespace antwika::editor
{

    bool Editor::handleAssignClick(const tilemap::Tile tile)
    {
        if (!selectedTile.has_value())
        {
            return false;
        }

        if (pickedTransition(tile))
        {
            return true;
        }

        if (assignMode.variantPicking && !isDecorLayer())
        {
            pickedVariant(tile);

            return true;
        }

        if (assignMode.basePicking && isDecorLayer())
        {
            pushUndo();
            ensureDecor();
            map.decor = decor::withBaseToggled(
                map.decor, *selectedTile, tile);
            rebuildWorld();

            return true;
        }

        if (assignMode.memberAssigning && isDecorLayer())
        {
            pushUndo();
            ensureDecor();
            map.decor = withMemberSet(
                map.decor, *selectedTile, assignMode.memberPicked, tile);
            assignMode.memberAssigning = false;
            rebuildWorld();

            return true;
        }

        if (assignMode.flipFrameAssigning
            && animationOf(map.flipAnimations, *selectedTile) != nullptr)
        {
            pushUndo();
            map.flipAnimations = withAnimationFrameSet(
                map.flipAnimations,
                *selectedTile,
                assignMode.flipFramePicked,
                tile);
            assignMode.flipFrameAssigning = false;
            atlasSheets.touch();

            return true;
        }

        return false;
    }

}
