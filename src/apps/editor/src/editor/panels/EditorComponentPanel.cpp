#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include <antwika/editor/ui/EditorLook.hpp>
#include <antwika/loadout/ComponentRow.hpp>
#include <antwika/loadout/ComponentValue.hpp>
#include <antwika/loadout/Descriptors.hpp>
#include <antwika/loadout/FieldKind.hpp>
#include <antwika/loadout/Role.hpp>

#include "antwika/editor/Editor.hpp"

#include "antwika/editor/ui/WidgetIds.hpp"

namespace antwika::editor
{

    namespace
    {

        [[nodiscard]] std::string_view shortNameOf(
            const std::string_view name)
        {
            const auto cut = name.rfind("::");

            return cut == std::string_view::npos
                       ? name
                       : name.substr(cut + 2);
        }

        [[nodiscard]] loadout::ComponentValue currentValueOf(
            const map::Character &character,
            const loadout::ComponentRow &row)
        {
            const auto foundEntry = character.componentValues.find(row.name);

            return foundEntry == character.componentValues.end()
                       ? row.fresh()
                       : foundEntry->second;
        }

        [[nodiscard]] std::size_t slotCountOf(
            const map::Character &character)
        {
            return std::min<std::size_t>(
                character.components.size(), kMaxComponentSlots);
        }

        struct ComponentPlace final
        {
            std::size_t slot;

            std::size_t field;
        };

        [[nodiscard]] std::optional<ComponentPlace> componentPlaceOf(
            const widget::WidgetId widgetId)
        {
            const auto first =
                static_cast<std::uint64_t>(kFirstComponentFieldWidget);
            const auto number = static_cast<std::uint64_t>(widgetId);

            if (number < first
                || number >= first
                                 + (kMaxComponentSlots
                                    * kMaxComponentFields))
            {
                return std::nullopt;
            }

            return ComponentPlace{
                .slot = (number - first) / kMaxComponentFields,
                .field = (number - first) % kMaxComponentFields};
        }

    }

    void Editor::layoutComponentSection(
        ui::Context &context, const std::size_t chosenCharacter)
    {
        const auto &character = document.map.characters.at(chosenCharacter);
        auto &tool = worldView.characterTool();

        panelTitle(context, "Components");

        const auto components = context.scrollColumn(
            antwika::ui::ScrollSpec{
                .widgetId = kComponentScrollWidget,
                .heightSizing = antwika::ui::kGrowSizing,
                .offset = tool.getInspectorScroll(),
                .dragging = tool.isInspectorTrackHeld()});

        for (std::size_t slot = 0; slot < slotCountOf(character); ++slot)
        {
            const auto &name = character.components.at(slot);
            const auto isShown = tool.getOpenComponent() == slot;

            {
                const auto head = context.row(
                    antwika::ui::ContainerSpec{
                        .widthSizing = antwika::ui::kGrowSizing});

                {
                    const auto nameCell = context.row(
                        antwika::ui::ContainerSpec{
                            .widthSizing = antwika::ui::kGrowSizing,
                            .heightSizing = antwika::ui::kGrowSizing,
                            .clips = true});

                    context.button(
                        std::string(shortNameOf(name)),
                        antwika::ui::ButtonSpec{
                            .widgetId = getComponentHeadWidget(slot),
                            .widthSizing = antwika::ui::kGrowSizing,
                            .fillColor = isShown
                                             ? kSelectionAccentColor
                                             : kGridLineColor});
                }

                context.button(
                    "x",
                    antwika::ui::ButtonSpec{
                        .widgetId = getComponentDropWidget(slot)});
            }

            if (!isShown)
            {
                continue;
            }

            const auto *row = loadout::getComponentRow(name);

            if (row == nullptr)
            {
                context.label(name, kGridLineColor);

                continue;
            }

            if (row->role == loadout::Role::Tag)
            {
                context.label("(tag)", kGridLineColor);

                continue;
            }

            const auto current = currentValueOf(character, *row);

            if (row->role == loadout::Role::Derived)
            {
                for (const auto &field : row->fields)
                {
                    context.label(
                        std::string(field.key) + ": "
                            + field.textOf(current),
                        kGridLineColor);
                }

                continue;
            }

            const auto fieldCount = std::min<std::size_t>(
                row->fields.size(), kMaxComponentFields);

            for (std::size_t place = 0; place < fieldCount; ++place)
            {
                const auto &field = row->fields[place];
                const auto fieldWidget =
                    getComponentFieldWidget(slot, place);

                if (field.kind == loadout::FieldKind::Flag)
                {
                    context.checkbox(
                        field.key,
                        antwika::ui::CheckboxSpec{
                            .widgetId = fieldWidget,
                            .checked =
                                field.valueOf(current).get<bool>()});

                    continue;
                }

                const auto editing =
                    tool.getEditingValueWidget() == fieldWidget;
                const auto valueRow = context.row(
                    antwika::ui::ContainerSpec{
                        .widthSizing = antwika::ui::kGrowSizing});

                context.label(field.key, kTextColor);
                context.textField(
                    antwika::ui::TextFieldSpec{
                        .widgetId = fieldWidget,
                        .text = editing ? tool.getPendingValueText()
                                        : field.textOf(current),
                        .focused = editing});
            }
        }

        context.button(
            "+ component",
            antwika::ui::ButtonSpec{
                .widgetId = kComponentAddOpenWidget,
                .widthSizing = antwika::ui::kGrowSizing});

        if (!tool.isAddListOpen())
        {
            return;
        }

        const auto rows = loadout::getComponentRows();

        for (std::size_t place = 0; place < rows.size(); ++place)
        {
            const auto &row = rows[place];
            const auto isListed =
                std::ranges::find(character.components, row.name)
                != character.components.end();

            if (row.role == loadout::Role::Derived || isListed)
            {
                continue;
            }

            context.button(
                std::string(shortNameOf(row.name)),
                antwika::ui::ButtonSpec{
                    .widgetId = getComponentAddWidget(place),
                    .widthSizing = antwika::ui::kGrowSizing});
        }
    }

    void Editor::carryComponentScroll(const ui::Interactions &interactions)
    {
        auto &tool = worldView.characterTool();

        if (interactions.scrollChange.has_value()
            && interactions.scrollChange->areaWidget
                   == kComponentScrollWidget)
        {
            tool.setInspectorScroll(interactions.scrollChange->line);
        }

        if (interactions.activatedWidget != widget::kNoWidget)
        {
            tool.setInspectorTrackHeld(
                interactions.areaPress.has_value()
                && interactions.areaPress->areaWidget
                       == kComponentScrollWidget);
        }
    }

    bool Editor::isInspectorHovered() const
    {
        const auto hoveredWidget = pointer.hoveredWidget;

        if (hoveredWidget == widget::kNoWidget)
        {
            return false;
        }

        if (hoveredWidget == kComponentAddOpenWidget
            || hoveredWidget == kComponentScrollWidget)
        {
            return true;
        }

        return hoveredWidget >= kFirstComponentHeadWidget
            && hoveredWidget < getWidgetAfter(
                   kFirstComponentFieldWidget,
                   kMaxComponentSlots * kMaxComponentFields);
    }

    bool Editor::componentWidgets(const ui::Interactions &interactions)
    {
        auto &tool = worldView.characterTool();

        carryComponentScroll(interactions);

        const auto chosenCharacter =
            worldView.characterTool().getChosenCharacter(
                document.map.characters.size());

        if (!chosenCharacter.has_value()
            || interactions.activatedWidget == widget::kNoWidget)
        {
            return false;
        }

        if (tool.getEditingValueWidget().has_value()
            && interactions.activatedWidget
                   != *tool.getEditingValueWidget())
        {
            tool.endValueEdit();
        }

        if (interactions.activatedWidget == kComponentScrollWidget)
        {
            return true;
        }

        auto &character = document.map.characters.at(*chosenCharacter);

        if (interactions.activatedWidget == kComponentAddOpenWidget)
        {
            tool.toggleAddList();

            return true;
        }

        for (std::size_t slot = 0; slot < slotCountOf(character); ++slot)
        {
            if (interactions.activatedWidget
                == getComponentHeadWidget(slot))
            {
                tool.toggleComponent(slot);

                return true;
            }

            if (interactions.activatedWidget
                == getComponentDropWidget(slot))
            {
                pushUndo();
                character.componentValues.erase(character.components.at(slot));
                character.components.erase(
                    std::next(
                        character.components.begin(),
                        static_cast<std::ptrdiff_t>(slot)));

                if (tool.getOpenComponent() == slot)
                {
                    tool.closeComponent();
                }
                else if (tool.getOpenComponent() > slot)
                {
                    tool.toggleComponent(*tool.getOpenComponent() - 1);
                }

                spawnCharacters();

                return true;
            }
        }

        const auto rows = loadout::getComponentRows();

        for (std::size_t place = 0;
             tool.isAddListOpen() && place < rows.size();
             ++place)
        {
            const auto &row = rows[place];
            const auto isListed =
                std::ranges::find(character.components, row.name)
                != character.components.end();

            if (interactions.activatedWidget
                    != getComponentAddWidget(place)
                || row.role == loadout::Role::Derived || isListed
                || character.components.size() >= kMaxComponentSlots)
            {
                continue;
            }

            pushUndo();
            character.components.emplace_back(row.name);

            if (row.role == loadout::Role::Valued)
            {
                character.componentValues.emplace(
                    std::string(row.name), row.fresh());
            }

            tool.closeAddList();
            spawnCharacters();

            return true;
        }

        const auto place = componentPlaceOf(interactions.activatedWidget);

        if (!place.has_value() || place->slot >= slotCountOf(character))
        {
            return false;
        }

        const auto *row =
            loadout::getComponentRow(character.components.at(place->slot));

        if (row == nullptr || row->role != loadout::Role::Valued
            || place->field >= row->fields.size())
        {
            return false;
        }

        const auto &field = row->fields[place->field];

        if (field.kind == loadout::FieldKind::Flag)
        {
            pushUndo();

            auto current = currentValueOf(character, *row);

            field.setFrom(
                current,
                nlohmann::json(!field.valueOf(current).get<bool>()));
            character.componentValues.insert_or_assign(
                std::string(row->name), current);
            spawnCharacters();

            return true;
        }

        if (tool.getEditingValueWidget()
            == interactions.activatedWidget)
        {
            return true;
        }

        pushUndo();
        focusedField = FocusedField::ComponentValue;
        tool.beginValueEdit(
            interactions.activatedWidget,
            field.textOf(currentValueOf(character, *row)));

        return true;
    }

    void Editor::commitComponentEdit()
    {
        auto &tool = worldView.characterTool();
        const auto editedWidget = tool.getEditingValueWidget();
        const auto chosenCharacter = tool.getChosenCharacter(
            document.map.characters.size());

        if (!editedWidget.has_value() || !chosenCharacter.has_value())
        {
            tool.endValueEdit();

            return;
        }

        const auto place = componentPlaceOf(*editedWidget);
        auto &character = document.map.characters.at(*chosenCharacter);

        if (!place.has_value() || place->slot >= slotCountOf(character))
        {
            tool.endValueEdit();

            return;
        }

        const auto *row =
            loadout::getComponentRow(character.components.at(place->slot));

        if (row == nullptr || row->role != loadout::Role::Valued
            || place->field >= row->fields.size())
        {
            tool.endValueEdit();

            return;
        }

        auto current = currentValueOf(character, *row);

        if (row->fields[place->field].setFromText(
                current, tool.getPendingValueText()))
        {
            character.componentValues.insert_or_assign(
                std::string(row->name), current);
            spawnCharacters();
        }

        tool.endValueEdit();
    }

}
