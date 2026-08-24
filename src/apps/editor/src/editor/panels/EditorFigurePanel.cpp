#include <antwika/editor/ui/EditorLook.hpp>
#include <antwika/light/PointLight.hpp>

#include "antwika/editor/editor/CarriedLight.hpp"
#include "antwika/editor/Editor.hpp"

namespace antwika::editor
{

    void Editor::layoutWorldRail(ui::Context &context)
    {
        const auto showExitPanel =
            preferences.tool == map::Tool::Exit && isWorldShown();
        const auto showFigures =
            preferences.tool == map::Tool::Figure
            && isWorldShown();

            if (showFigures)
            {
                const auto figuresPanel = context.column(
                    antwika::ui::ContainerSpec{
                        .widthSizing = antwika::ui::kGrowSizing,
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

                    context.label("Figures", kTextColor);
                    context.spacer(antwika::ui::kGrowSizing);
                    context.button(
                        "+",
                        antwika::ui::ButtonSpec{
                            .widgetId = antwika::editor::
                                kAddFigureWidget});
                    context.button(
                        "x",
                        antwika::ui::ButtonSpec{
                            .widgetId = antwika::editor::
                                kRemoveFigureWidget});
                }

                for (std::size_t index = 0;
                     index < document.map.characters.size();
                     ++index)
                {
                    context.button(
                        document.map.characters.at(index).name.empty()
                            ? "Figure "
                                  + std::to_string(index)
                            : document.map.characters.at(index).name,
                        antwika::ui::ButtonSpec{
                            .widgetId = getFigureWidget(index),
                            .widthSizing = antwika::ui::kGrowSizing,
                            .fillColor = worldView.figureTool.chosenIndex == index
                                       ? kSelectionAccentColor
                                       : kGridLineColor});
                }

                if (worldView.figureTool.chosenIndex.has_value()
                    && *worldView.figureTool.chosenIndex < document.map.characters.size())
                {
                    context.textField(
                        antwika::ui::TextFieldSpec{
                            .widgetId = antwika::editor::
                                kFigureNameWidget,
                            .text = document.map.characters
                                        .at(*worldView.figureTool.chosenIndex)
                                        .name,
                            .placeholder = "name",
                            .focused = focusedField
                                       == FocusedField::FigureName});
                    context.checkbox(
                        "carries a lamp",
                        antwika::ui::CheckboxSpec{
                            .widgetId = antwika::editor::
                                kFigureLampWidget,
                            .checked = carriesLight(
                                document.map.characters.at(
                                    *worldView.figureTool.chosenIndex))});

                    for (const auto &line :
                         document.map.characters.at(*worldView.figureTool.chosenIndex)
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
                                    kFigureLineWidget,
                                .text = worldView.figureTool.pendingLine,
                                .placeholder =
                                    "a line to say",
                                .focused =
                                    focusedField
                                    == FocusedField::FigureLine});
                        context.button(
                            "+",
                            antwika::ui::ButtonSpec{
                                .widgetId = antwika::editor::
                                    kFigureLineAddWidget});
                    }
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
                context.checkbox(
                    "Needs a key",
                    antwika::ui::CheckboxSpec{
                        .widgetId = antwika::editor::
                            kExitLockedWidget,
                        .checked = document.map.exitLocked});
            }

            if (preferences.tool == map::Tool::Lamp
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

    void Editor::layoutFigureChooser(ui::Context &context)
    {
            if (viewChoice.activeView == map::View::Character)
            {
                const auto figureChooserPanel = context.column(
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
                            ? "Figure "
                                  + std::to_string(index)
                            : document.map.characters.at(index).name,
                        antwika::ui::ButtonSpec{
                            .widgetId = getFigureWidget(index),
                            .widthSizing = antwika::ui::kGrowSizing,
                            .fillColor = characterView.getEditing() == index
                                       ? kSelectionAccentColor
                                       : kGridLineColor});
                }
            }
    }

    bool Editor::figureRosterWidgets(
        const ui::Interactions &interactions)
    {
        auto consumedKey = false;

        for (std::size_t index = 0; index < document.map.characters.size();
             ++index)
        {
            if (interactions.activatedWidget != getFigureWidget(index))
            {
                continue;
            }

            consumedKey = true;

            if (viewChoice.activeView == map::View::Character)
            {
                characterView.switchTo(viewportRenderer, rosterSkins, index);

                continue;
            }

            worldView.figureTool.chosenIndex = index;
            worldView.figureTool.placed = false;
        }

        if (interactions.activatedWidget
            == antwika::editor::kAddFigureWidget)
        {
            pushUndo();
            document.map.characters.push_back(
                map::Character{
                    .name = "Figure "
                            + std::to_string(
                                document.map.characters.size())});
            worldView.figureTool.chosenIndex = document.map.characters.size() - 1;
            worldView.figureTool.placed = false;
            spawnRoster();
            loadCharacterSkins();
            consumedKey = true;
        }

        if (interactions.activatedWidget
                == antwika::editor::kRemoveFigureWidget
            && worldView.figureTool.chosenIndex.has_value()
            && *worldView.figureTool.chosenIndex < document.map.characters.size()
            && !document.map.characters.at(*worldView.figureTool.chosenIndex).player)
        {
            pushUndo();
            document.map.characters.erase(
                std::next(
                    document.map.characters.begin(),
                    static_cast<std::ptrdiff_t>(
                        *worldView.figureTool.chosenIndex)));
            worldView.figureTool.chosenIndex.reset();
            spawnRoster();
            loadCharacterSkins();
            consumedKey = true;
        }

        if (interactions.activatedWidget
                == antwika::editor::kFigureNameWidget
            && worldView.figureTool.chosenIndex.has_value())
        {
            pushUndo();
            focusedField = FocusedField::FigureName;
            consumedKey = true;
        }

        if (interactions.activatedWidget
                == antwika::editor::kFigureLampWidget
            && worldView.figureTool.chosenIndex.has_value()
            && *worldView.figureTool.chosenIndex < document.map.characters.size())
        {
            pushUndo();
            toggleCarriedLight(document.map.characters.at(*worldView.figureTool.chosenIndex));
            consumedKey = true;
        }

        if (interactions.activatedWidget
            == antwika::editor::kFigureLineWidget)
        {
            focusedField = FocusedField::FigureLine;
            consumedKey = true;
        }

        if (interactions.activatedWidget
                == antwika::editor::kFigureLineAddWidget
            && worldView.figureTool.chosenIndex.has_value()
            && *worldView.figureTool.chosenIndex < document.map.characters.size()
            && !worldView.figureTool.pendingLine.empty())
        {
            pushUndo();
            document.map.characters.at(*worldView.figureTool.chosenIndex)
                .dialogue.push_back(worldView.figureTool.pendingLine);
            worldView.figureTool.pendingLine.clear();
            focusedField = FocusedField::Nothing;
            consumedKey = true;
        }

        return consumedKey;
    }

}
