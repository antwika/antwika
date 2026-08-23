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
            document.map.decor = decor::withBaseToggled(
                document.map.decor, *selectedTile, tile);
            rebuildWorld();

            return true;
        }

        if (assignMode.memberAssigning && isDecorLayer())
        {
            pushUndo();
            ensureDecor();
            document.map.decor = withMemberSet(
                document.map.decor, *selectedTile, assignMode.memberPicked,
                tile);
            assignMode.memberAssigning = false;
            rebuildWorld();

            return true;
        }

        if (assignMode.flipFrameAssigning
            && animationOf(document.map.flipAnimations,
                *selectedTile) != nullptr)
        {
            pushUndo();
            document.map.flipAnimations = withAnimationFrameSet(
                document.map.flipAnimations,
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
