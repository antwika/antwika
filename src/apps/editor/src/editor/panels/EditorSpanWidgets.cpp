#include <antwika/decor/Decor.hpp>

#include "antwika/editor/Editor.hpp"

namespace antwika::editor
{

    bool Editor::nudgeSpan(const int acrossStep, const int downStep)
    {
        if (!stroke.selectedTile.has_value() || !isDecorLayer(chosenLayer))
        {
            return false;
        }

        pushUndo();
        ensureDecor();

        const auto *decor = decor::decorOf(
            document.map.decor, *stroke.selectedTile);

        document.map.decor = getWithSpanSet(
            document.map.decor,
            *stroke.selectedTile,
            static_cast<std::uint8_t>(
                std::max(decor->width + acrossStep, 1)),
            static_cast<std::uint8_t>(
                std::max(decor->height + downStep, 1)));
        assignMode.memberPicked = 0;
        assignMode.memberAssigning = false;
        rebuildWorld();

        return true;
    }

}
