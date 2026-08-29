#include <antwika/editor/ui/EditorLook.hpp>
#include <antwika/gameplay/ComponentNames.hpp>
#include <antwika/light/PointLight.hpp>

#include "antwika/editor/editor/CarriedLight.hpp"
#include "antwika/editor/Editor.hpp"

#include "antwika/editor/ui/WidgetIds.hpp"

namespace antwika::editor
{

    void Editor::layoutWorldRail(ui::Context &context)
    {
        const auto showExitPanel =
            preferences.tool == Tool::Exit && isWorldShown();
        const auto showCharacters =
            preferences.tool == Tool::Character
            && isWorldShown();

            if (showCharacters)
            {
                const auto charactersPanel = context.column(
                    antwika::ui::ContainerSpec{
                        .widthSizing = antwika::ui::kGrowSizing,
                        .heightSizing = antwika::ui::kGrowSizing,
                        .backgroundColor = kPanelColor,
                        .padding = kPanelPadding});

                {
                    const auto heading = context.row(
                        antwika::ui::ContainerSpec{
                            .widthSizing = antwika::ui::kGrowSizing,
                            .crossAlignment = antwika::ui::
                                Alignment::Center,
                            .backgroundColor = kTitleBarColor,
                            .padding = kPanelPadding});

                    context.label("Characters", kTextColor);
                    context.spacer(antwika::ui::kGrowSizing);
                    context.button(
                        "+",
                        antwika::ui::ButtonSpec{
                            .widgetId = antwika::editor::
                                kAddCharacterWidget});
                    context.button(
                        "x",
                        antwika::ui::ButtonSpec{
                            .widgetId = antwika::editor::
                                kRemoveCharacterWidget});
                }

                for (std::size_t index = 0;
                     index < document.map.characters.size();
                     ++index)
                {
                    context.button(
                        document.map.characters.at(index).name.empty()
                            ? "Character "
                                  + std::to_string(index)
                            : document.map.characters.at(index).name,
                        antwika::ui::ButtonSpec{
                            .widgetId = getCharacterWidget(index),
                            .widthSizing = antwika::ui::kGrowSizing,
                            .fillColor =
                                worldView.characterTool().isChosen(index)
                                       ? kSelectionAccentColor
                                       : kGridLineColor});
                }

                if (const auto chosenCharacter =
                        worldView.characterTool().getChosenCharacter(
                            document.map.characters.size());
                    chosenCharacter.has_value())
                {
                    context.textField(
                        antwika::ui::TextFieldSpec{
                            .widgetId = antwika::editor::
                                kCharacterNameWidget,
                            .text = document.map.characters
                                        .at(*chosenCharacter)
                                        .name,
                            .placeholder = "name",
                            .focused = focusedField
                                       == FocusedField::CharacterName});
                    context.checkbox(
                        "carries a lamp",
                        antwika::ui::CheckboxSpec{
                            .widgetId = antwika::editor::
                                kCharacterLampWidget,
                            .checked = carriesLight(
                                document.map.characters.at(
                                    *chosenCharacter))});

                    for (const auto &line :
                         document.map.characters.at(*chosenCharacter)
                             .dialogue)
                    {
                        context.label(line, kGridLineColor);
                    }

                    {
                        const auto lineRow = context.row(
                            antwika::ui::ContainerSpec{
                                .widthSizing =
                                    antwika::ui::kGrowSizing});

                        context.textField(
                            antwika::ui::TextFieldSpec{
                                .widgetId = antwika::editor::
                                    kCharacterLineWidget,
                                .text = worldView.characterTool()
                                            .getPendingLine(),
                                .placeholder =
                                    "a line to say",
                                .focused =
                                    focusedField
                                    == FocusedField::CharacterLine});
                        context.button(
                            "+",
                            antwika::ui::ButtonSpec{
                                .widgetId = antwika::editor::
                                    kCharacterLineAddWidget});
                    }

                    layoutComponentSection(context, *chosenCharacter);
                }
            }

            if (showExitPanel)
            {
                const auto exitPanel = context.column(
                    antwika::ui::ContainerSpec{
                        .widthSizing = antwika::ui::kGrowSizing,
                        .backgroundColor = kPanelColor,
                        .padding = kPanelPadding});

                panelTitle(context, "Exit leads to");
                context.textField(
                    antwika::ui::TextFieldSpec{
                        .widgetId = antwika::editor::
                            kExitTargetWidget,
                        .text = document.map.exitTarget,
                        .placeholder = "Closes the game",
                        .focused = focusedField == FocusedField::ExitTarget});
            }

            if (preferences.tool == Tool::Lamp
                && isWorldShown())
            {
                const auto ambientPanel = context.column(
                    antwika::ui::ContainerSpec{
                        .widthSizing = antwika::ui::kGrowSizing,
                        .backgroundColor = kPanelColor,
                        .padding = kPanelPadding});

                panelTitle(
                    context,
                    "Ambient "
                        + std::to_string(document.map.ambient));
                context.slider(
                    antwika::ui::SliderSpec{
                        .widgetId = antwika::editor::
                            kAmbientWidget,
                        .value = document.map.ambient,
                        .range = 100,
                        .dragging =
                            slidingWidget
                            == antwika::editor::
                                kAmbientWidget});
            }
    }

    void Editor::layoutCharacterChooser(ui::Context &context)
    {
            if (viewChoice.activeView == View::Character)
            {
                const auto characterChooserPanel = context.column(
                    antwika::ui::ContainerSpec{
                        .widthSizing = antwika::ui::kGrowSizing,
                        .backgroundColor = kPanelColor,
                        .padding = kPanelPadding});

                panelTitle(context, "Drawing");

                for (std::size_t index = 0;
                     index < document.map.characters.size();
                     ++index)
                {
                    context.button(
                        document.map.characters.at(index).name.empty()
                            ? "Character "
                                  + std::to_string(index)
                            : document.map.characters.at(index).name,
                        antwika::ui::ButtonSpec{
                            .widgetId = getCharacterWidget(index),
                            .widthSizing = antwika::ui::kGrowSizing,
                            .fillColor = characterView.getEditing() == index
                                       ? kSelectionAccentColor
                                       : kGridLineColor});
                }
            }
    }

    bool Editor::characterWidgets(
        const ui::Interactions &interactions)
    {
        auto consumedKey = false;

        for (std::size_t index = 0; index < document.map.characters.size();
             ++index)
        {
            if (interactions.activatedWidget != getCharacterWidget(index))
            {
                continue;
            }

            consumedKey = true;

            if (viewChoice.activeView == View::Character)
            {
                characterView.switchTo(viewportRenderer, characterSkins, index);

                continue;
            }

            worldView.characterTool().choose(index);
        }

        if (interactions.activatedWidget
            == antwika::editor::kAddCharacterWidget)
        {
            const auto characterNames =
                antwika::gameplay::getCharacterComponentNames();

            pushUndo();
            document.map.characters.push_back(
                map::Character{
                    .name = "Character "
                            + std::to_string(
                                document.map.characters.size()),
                    .components = std::vector<std::string>(
                        characterNames.begin(), characterNames.end())});
            worldView.characterTool().choose(
                document.map.characters.size() - 1);
            spawnCharacters();
            loadCharacterSkins();
            consumedKey = true;
        }

        if (const auto chosenCharacter =
                worldView.characterTool().getChosenCharacter(
                    document.map.characters.size());
            interactions.activatedWidget
                == antwika::editor::kRemoveCharacterWidget
            && chosenCharacter.has_value()
            && !document.map.characters.at(*chosenCharacter).player)
        {
            pushUndo();
            document.map.characters.erase(
                std::next(
                    document.map.characters.begin(),
                    static_cast<std::ptrdiff_t>(*chosenCharacter)));
            worldView.characterTool().dropChoice();
            spawnCharacters();
            loadCharacterSkins();
            consumedKey = true;
        }

        if (interactions.activatedWidget
                == antwika::editor::kCharacterNameWidget
            && worldView.characterTool().hasChoice())
        {
            pushUndo();
            focusedField = FocusedField::CharacterName;
            consumedKey = true;
        }

        if (interactions.activatedWidget
            == antwika::editor::kCharacterLineWidget)
        {
            focusedField = FocusedField::CharacterLine;
            consumedKey = true;
        }

        if (const auto chosenCharacter =
                worldView.characterTool().getChosenCharacter(
                    document.map.characters.size());
            interactions.activatedWidget
                == antwika::editor::kCharacterLineAddWidget
            && chosenCharacter.has_value()
            && !worldView.characterTool().getPendingLine().empty())
        {
            pushUndo();
            document.map.characters.at(*chosenCharacter)
                .dialogue.push_back(
                    worldView.characterTool().takePendingLine());
            focusedField = FocusedField::Nothing;
            consumedKey = true;
        }

        return consumedKey;
    }

}
