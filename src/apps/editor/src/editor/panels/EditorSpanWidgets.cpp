#include <antwika/decor/Decor.hpp>

#include "antwika/editor/Editor.hpp"

namespace antwika::editor
{

    bool Editor::spanWidgets(
        const ui::Interactions &interactions)
    {
        if (!stroke.selectedTile.has_value() || !isDecorLayer(chosenLayer))
        {
            return false;
        }

        const auto nudgeSpan =
            [this](const int acrossStep, const int downStep)
        {
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
        };

        if (interactions.activatedWidget
            == decor::kSpanAcrossLessWidget)
        {
            nudgeSpan(-1, 0);

            return true;
        }

        if (interactions.activatedWidget
            == decor::kSpanAcrossMoreWidget)
        {
            nudgeSpan(1, 0);

            return true;
        }

        if (interactions.activatedWidget
            == decor::kSpanDownLessWidget)
        {
            nudgeSpan(0, -1);

            return true;
        }

        if (interactions.activatedWidget
            == decor::kSpanDownMoreWidget)
        {
            nudgeSpan(0, 1);

            return true;
        }

        const auto *decor =
            decor::decorOf(document.map.decor, *stroke.selectedTile);

        for (std::size_t place = 1;
             decor != nullptr && place < decor->spanTiles.size();
             ++place)
        {
            if (interactions.activatedWidget
                != decor::getMemberWidget(place))
            {
                continue;
            }

            clearAssignModes();
            assignMode.memberPicked = place;
            assignMode.memberAssigning = true;

            return true;
        }

        return false;
    }

}
