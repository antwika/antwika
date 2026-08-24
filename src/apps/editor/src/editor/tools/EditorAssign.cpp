#include <antwika/decor/Decor.hpp>
#include <antwika/tilemap/Tilemap.hpp>

#include "antwika/editor/Editor.hpp"

namespace antwika::editor
{

    bool Editor::consumeAssignClick(const tilemap::Tile tile)
    {
        if (!stroke.selectedTile.has_value())
        {
            return false;
        }

        if (pickedTransition(tile))
        {
            return true;
        }

        if (assignMode.variantPicking && !isDecorLayer(chosenLayer))
        {
            pickedVariant(tile);

            return true;
        }

        if (assignMode.basePicking && isDecorLayer(chosenLayer))
        {
            pushUndo();
            ensureDecor();
            document.map.decor = decor::getWithBaseToggled(
                document.map.decor, *stroke.selectedTile, tile);
            rebuildWorld();

            return true;
        }

        if (assignMode.memberAssigning && isDecorLayer(chosenLayer))
        {
            pushUndo();
            ensureDecor();
            document.map.decor = getWithMemberSet(
                document.map.decor, *stroke.selectedTile, assignMode.memberPicked,
                tile);
            assignMode.memberAssigning = false;
            rebuildWorld();

            return true;
        }

        if (assignMode.flipFrameAssigning
            && animationOf(document.map.flipAnimations,
                *stroke.selectedTile) != nullptr)
        {
            pushUndo();
            document.map.flipAnimations = getWithAnimationFrameSet(
                document.map.flipAnimations,
                *stroke.selectedTile,
                assignMode.flipFramePicked,
                tile);
            assignMode.flipFrameAssigning = false;
            atlasSheets.touch();

            return true;
        }

        return false;
    }

}
