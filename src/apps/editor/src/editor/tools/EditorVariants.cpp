#include <antwika/decor/Decor.hpp>
#include <antwika/editor/ui/EditorLook.hpp>
#include <antwika/tilemap/Tilemap.hpp>

#include "antwika/editor/Editor.hpp"

namespace antwika::editor
{

    std::uint8_t Editor::variantWeightOf(const tilemap::Tile tile) const
    {
        const auto *leads = groupLedBy(map.familyGroups, tile);

        if (leads != nullptr)
        {
            return leads->weight;
        }

        const auto *family = groupContaining(map.familyGroups, tile);

        for (const auto &member :
             family != nullptr ? family->variants
                     : std::vector<decor::VariantMember>{})
        {
            if (member.tile == tile)
            {
                return member.weight;
            }
        }

        return decor::kFullFrequency;
    } // GCOVR_EXCL_LINE

    bool Editor::blockedAsVariant()
    {
        if (isDecorLayer() || !selectedTile.has_value()
            || groupContaining(map.familyGroups, *selectedTile) == nullptr)
        {
            return false;
        }

        showStatus("a variant borrows its leader's rules", true);

        return true;
    }

    void Editor::pickedVariant(const tilemap::Tile tile)
    {
        const auto canonicalTile = *selectedTile;
        const auto *family = groupContaining(map.familyGroups, tile);
        const auto mine =
            family != nullptr && family->canonicalTile == canonicalTile;

        if (!mine
            && !canBeVariantOf(
                map.familyGroups,
                map.rules,
                map.decor,
                canonicalTile,
                tile))
        {
            showStatus(
                "only a rule-less tile of the leader's "
                "atlas may stand as a variant",
                true);

            return;
        }

        pushUndo();
        map.familyGroups =
            withVariantToggled(map.familyGroups, canonicalTile, tile);
        rebuildWorld();
    }

    bool Editor::variantWidgets(
        const ui::Interactions &interactions)
    {
        auto consumedKey = false;

        if (interactions.activatedWidget
            == decor::kVariantChoiceWidget)
        {
            const auto was = assignMode.variantPicking;

            clearAssignModes();
            assignMode.variantPicking = !was;
            consumedKey = true;
        }

        if (interactions.activatedWidget
                == decor::kGoToCanonicalWidget
            && selectedTile.has_value())
        {
            selectedTile = canonicalTileOf(map.familyGroups, *selectedTile);
            assignMode.variantPicking = false;
            consumedKey = true;
        }

        return consumedKey;
    }

    void Editor::layoutVariantRail(ui::Context &context)
    {
        if (activeView != map::View::Atlases || !selectedTile.has_value()
            || isDecorLayer())
        {
            return;
        }

        const auto *family =
            groupContaining(map.familyGroups, *selectedTile);
        const auto variantsPanel = context.column(
            antwika::ui::ContainerSpec{
                .widthSizing = antwika::ui::kGrowSizing,
                .backgroundColor = kPanelColor,
                .padding = kPanelPadding});

        panelTitle(context, "Variants");

        if (family != nullptr)
        {
            context.button(
                "variant of",
                antwika::ui::ButtonSpec{
                    .widgetId = decor::kGoToCanonicalWidget,
                    .widthSizing = antwika::ui::kGrowSizing});
        }
        else
        {
            context.checkbox(
                "pick variants",
                antwika::ui::CheckboxSpec{
                    .widgetId = decor::kVariantChoiceWidget,
                    .checked = assignMode.variantPicking});
        }

        if (family != nullptr
            || groupLedBy(map.familyGroups, *selectedTile) != nullptr)
        {
            context.label(
                "weight "
                    + std::to_string(
                        variantWeightOf(*selectedTile)),
                kTextColor);
            context.slider(
                antwika::ui::SliderSpec{
                    .widgetId = decor::kVariantWeightWidget,
                    .value = variantWeightOf(*selectedTile),
                    .range = decor::kFullFrequency,
                    .dragging =
                        slidingWidget
                        == decor::
                            kVariantWeightWidget});
        }
    }

}
