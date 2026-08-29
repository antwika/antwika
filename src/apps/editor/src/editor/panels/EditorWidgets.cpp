#include "antwika/editor/Editor.hpp"
#include "antwika/editor/ui/WidgetCatalog.hpp"

namespace antwika::editor
{

    namespace
    {
        constexpr std::uint32_t kPlanNoticeTicks = 120;
    }

    bool Editor::consumeWidgets(
        const ui::Interactions &interactions)
    {
        auto consumedKey = false;

        if (consumeModalWidgets(interactions))
        {
            return true;
        }

        if (auto *view = viewNow(); view != nullptr)
        {
            std::optional<std::string> notice;
            const auto took =
                view->takeWidgets(interactions, viewContextNow(), notice);

            if (notice.has_value())
            {
                showStatus(*notice, false, kPlanNoticeTicks);
            }

            if (took)
            {
                return true;
            }
        }

        if (interactions.activatedWidget != antwika::widget::kNoWidget
            && interactions.activatedWidget
                   != getWidgetForField(focusedField))
        {
            focusedField = FocusedField::Nothing;
        }

        const auto &catalog = getWidgetCatalog();

        for (const auto &row : catalog.soloRows)
        {
            if (interactions.activatedWidget == row.widget
                && row.activation != nullptr && row.activation(*this))
            {
                consumedKey = true;
            }
        }

        for (const auto &family : catalog.familyRows)
        {
            if (family.activation == nullptr)
            {
                continue;
            }

            const auto placeEnd = widget_catalog::placeEndIn(family, *this);

            for (auto place = family.firstPlace; place < placeEnd; ++place)
            {
                if (interactions.activatedWidget == family.widgetAt(place)
                    && family.activation(*this, place))
                {
                    consumedKey = true;
                }
            }
        }

        if (characterWidgets(interactions))
        {
            consumedKey = true;
        }

        if (componentWidgets(interactions))
        {
            consumedKey = true;
        }

        if (inkPanel.consumePaletteWidgets(interactions, pointer, tick))
        {
            consumedKey = true;
        }

        return consumedKey;
    }

}
