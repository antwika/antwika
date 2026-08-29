#include <algorithm>
#include <array>
#include <optional>

#include <antwika/editor/ui/EditorLook.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/KeyText.hpp>

#include "antwika/editor/Editor.hpp"

#include "antwika/editor/ui/WidgetIds.hpp"

namespace antwika::editor
{

    widget::WidgetId Editor::getWidgetForField(
        const FocusedField focusedField) const
    {
        switch (focusedField)
        {
        case FocusedField::ExitTarget:
            return antwika::editor::kExitTargetWidget;
        case FocusedField::CharacterName:
            return antwika::editor::kCharacterNameWidget;
        case FocusedField::CharacterLine:
            return antwika::editor::kCharacterLineWidget;
        case FocusedField::PlanTitle:
            return antwika::editor::kPlanTitleWidget;
        case FocusedField::PlanBody:
            return antwika::editor::kPlanBodyWidget;
        case FocusedField::ComponentValue:
            return worldView.getCharacterTool()
                .getEditingValueWidget()
                .value_or(antwika::widget::kNoWidget);
        case FocusedField::MarkerAxis:
            return markerPick.editingAxis.has_value()
                       ? getMarkerFieldWidget(*markerPick.editingAxis)
                       : antwika::widget::kNoWidget;
        case FocusedField::EntityAxis:
            return entityPick.editingAxis.has_value()
                       ? getEntityFieldWidget(*entityPick.editingAxis)
                       : antwika::widget::kNoWidget;
        case FocusedField::Nothing:
            break;
        }

        return antwika::widget::kNoWidget;
    }

    bool Editor::layoutModals(ui::Context &context)
    {
        if (dialogs.quitConfirmOpen)
        {
            const auto asking = context.row(
                antwika::ui::ContainerSpec{
                    .widthSizing = antwika::ui::kGrowSizing,
                    .heightSizing = antwika::ui::kGrowSizing,
                    .crossAlignment = antwika::ui::Alignment::Center});

            context.spacer(antwika::ui::kGrowSizing);

            {
                const auto sheet = context.column(
                    antwika::ui::ContainerSpec{
                        .widthSizing = antwika::ui::getFixedSize(
                            kPickerWidth * kUiScale * 2.0F),
                        .backgroundColor = kPanelColor,
                        .padding = kPanelPadding});

                panelTitle(context, "Quit without saving?");

                const auto buttonRow = context.row(
                    antwika::ui::ContainerSpec{
                        .widthSizing = antwika::ui::kGrowSizing});

                context.button(
                    "Save and quit",
                    antwika::ui::ButtonSpec{
                        .widgetId = antwika::editor::kQuitAndSaveWidget,
                        .widthSizing = antwika::ui::kGrowSizing});
                context.button(
                    "Quit",
                    antwika::ui::ButtonSpec{
                        .widgetId = antwika::editor::kQuitConfirmWidget,
                        .widthSizing = antwika::ui::kGrowSizing});
                context.button(
                    "Cancel",
                    antwika::ui::ButtonSpec{
                        .widgetId = antwika::editor::kQuitCancelWidget,
                        .widthSizing = antwika::ui::kGrowSizing});
            }

            context.spacer(antwika::ui::kGrowSizing);
        }
        if (keyBench.panelShown)
        {
            const auto asking = context.row(
                antwika::ui::ContainerSpec{
                    .widthSizing = antwika::ui::kGrowSizing,
                    .heightSizing = antwika::ui::kGrowSizing,
                    .crossAlignment = antwika::ui::Alignment::Center});

            context.spacer(antwika::ui::kGrowSizing);

            {
                const auto sheet = context.column(
                    antwika::ui::ContainerSpec{
                        .widthSizing = antwika::ui::getFixedSize(
                            kPickerWidth * kUiScale * 4.0F),
                        .backgroundColor = kPanelColor,
                        .padding = kPanelPadding});

                panelTitle(
                    context,
                    keyBench.rebindingAction.has_value()
                        ? "Press the keys for it - escape "
                          "lets it be"
                        : "Keys - press a row, then the keys");

                const auto actionList = getAllActions();

                {
                    const auto ranks = context.row(
                        antwika::ui::ContainerSpec{
                            .widthSizing = antwika::ui::kGrowSizing});

                    for (std::size_t rank = 0; rank < 4;
                         ++rank)
                    {
                        const auto side = context.column(
                            antwika::ui::ContainerSpec{
                                .widthSizing = antwika::ui::kGrowSizing});

                        const auto perRank =
                            (kActionCount + 3) / 4;

                        for (std::size_t index = rank * perRank;
                             index < (rank + 1) * perRank
                             && index < kActionCount;
                             ++index)
                        {
                            const auto act = actionList.at(index);

                            context.button(
                                std::string(getActionLabel(act))
                                    + " - "
                                    + (keyBench.rebindingAction == act
                                                        ? "..."
                                                        : getChordName(
                                               keyBench.getBindings().at(
                                                   act))),
                                antwika::ui::ButtonSpec{
                                    .widgetId = getKeyRowWidget(index),
                                    .widthSizing =
                                        antwika::ui::kGrowSizing,
                                    .fillColor = keyBench.rebindingAction == act
                                               ? kSelectionAccentColor
                                               : kGridLineColor});
                        }
                    }
                }

                const auto buttonRow = context.row(
                    antwika::ui::ContainerSpec{
                        .widthSizing = antwika::ui::kGrowSizing});

                context.button(
                    "done",
                    antwika::ui::ButtonSpec{
                        .widgetId = antwika::editor::kKeysDoneWidget,
                        .widthSizing = antwika::ui::kGrowSizing});
                context.button(
                    "defaults",
                    antwika::ui::ButtonSpec{
                        .widgetId = antwika::editor::kKeysResetWidget,
                        .widthSizing = antwika::ui::kGrowSizing});
            }

            context.spacer(antwika::ui::kGrowSizing);
        }

        return dialogs.quitConfirmOpen || keyBench.panelShown;
    }

    bool Editor::consumeModalWidgets(
        const ui::Interactions &interactions)
    {
        if (dialogs.quitConfirmOpen)
        {
            if (interactions.activatedWidget
                == antwika::editor::kQuitAndSaveWidget)
            {
                saveCurrentMap();

                if (!document.isDirty())
                {
                    running = false;
                }
            }

            if (interactions.activatedWidget
                == antwika::editor::kQuitConfirmWidget)
            {
                running = false;
            }

            if (interactions.activatedWidget
                == antwika::editor::kQuitCancelWidget)
            {
                dialogs.quitConfirmOpen = false;
            }

            return true;
        }

        if (keyBench.panelShown)
        {
            const auto actionList = getAllActions();

            for (std::size_t index = 0; index < kActionCount; ++index)
            {
                if (interactions.activatedWidget == getKeyRowWidget(index))
                {
                    keyBench.rebindingAction = actionList.at(index);
                }
            }

            if (interactions.activatedWidget
                == antwika::editor::kKeysDoneWidget)
            {
                keyBench.panelShown = false;
                keyBench.rebindingAction.reset();
            }

            if (interactions.activatedWidget
                == antwika::editor::kKeysResetWidget)
            {
                keyBench.takeBindings(getDefaultChords());
                keyBench.rebindingAction.reset();
                saveChords(keyBench.getBindings(), getChordsPath());
            }

            return true;
        }

        return false;
    }

    namespace
    {
        struct MovingRow final
        {
            input::Key pressedKey;
            ui::Key plainKey;
            ui::Key shiftedKey;
        };

        constexpr std::array kMovingRows{
            MovingRow{
                input::Key::Backspace, ui::Key::Backspace,
                ui::Key::Backspace},
            MovingRow{
                input::Key::Delete, ui::Key::Delete, ui::Key::Delete},
            MovingRow{
                input::Key::ArrowLeft, ui::Key::MoveLeft,
                ui::Key::SelectLeft},
            MovingRow{
                input::Key::ArrowRight, ui::Key::MoveRight,
                ui::Key::SelectRight},
            MovingRow{
                input::Key::ArrowUp, ui::Key::MoveUp, ui::Key::SelectUp},
            MovingRow{
                input::Key::ArrowDown, ui::Key::MoveDown,
                ui::Key::SelectDown},
            MovingRow{
                input::Key::Home, ui::Key::MoveLineStart,
                ui::Key::SelectLineStart},
            MovingRow{
                input::Key::End, ui::Key::MoveLineEnd,
                ui::Key::SelectLineEnd}};

        [[nodiscard]] std::optional<ui::Key> getMovingKey(
            const input::Key key, const bool shiftHeld)
        {
            const auto foundRow =
                std::ranges::find(kMovingRows, key, &MovingRow::pressedKey);

            if (foundRow == kMovingRows.end())
            {
                return std::nullopt;
            }

            return shiftHeld ? foundRow->shiftedKey : foundRow->plainKey;
        }
    }

    bool Editor::consumeTextInput(const input::KeyPressed &pressedKey)
    {
        if (fileChooser.fileDialog.has_value())
        {
            const auto typedText = antwika::input::getCharTypedBy(
                pressedKey.key, getHeldModifiers().shift);

            if (!typedText.empty())
            {
                keyBench.typedThisFrame += typedText;
                keyBench.keysNow.push_back(
                    antwika::ui::Key::Character);
            }

            if (pressedKey.key == input::Key::Backspace)
            {
                keyBench.keysNow.push_back(
                    antwika::ui::Key::Backspace);
            }

            if (pressedKey.key == input::Key::Enter)
            {
                confirmFileDialog();
            }

            if (pressedKey.key == input::Key::Escape)
            {
                fileChooser.cancel();
            }

            return true;
        }

        if (inkPanel.inkPicker.editingInk.has_value())
        {
            const auto typedText = antwika::input::getCharTypedBy(
                pressedKey.key, getHeldModifiers().shift);

            if (!typedText.empty())
            {
                keyBench.typedThisFrame += typedText;
                keyBench.keysNow.push_back(
                    antwika::ui::Key::Character);
            }

            if (pressedKey.key == input::Key::Backspace)
            {
                keyBench.keysNow.push_back(
                    antwika::ui::Key::Backspace);
            }

            if (pressedKey.key == input::Key::Enter)
            {
                inkPanel.inkPicker.editingInk.reset();
            }

            if (pressedKey.key == input::Key::Escape)
            {
                inkPanel.recolorInk(inkPanel.inkPicker.inkBeforeEditColor);

                if (*inkPanel.inkPicker.editingInk < document.map.glows.size())
                {
                    document.map.glows.at(*inkPanel.inkPicker.editingInk) =
                        inkPanel.inkPicker.glowBeforeEdit;
                    atlasSheets.touch();
                }

                inkPanel.inkPicker.editingInk.reset();
            }

            return true;
        }

        if (focusedField != FocusedField::Nothing)
        {
            const auto keyModifiers = getHeldModifiers();
            const auto typedText =
                keyModifiers.control
                    ? std::string{}
                    : antwika::input::getCharTypedBy(
                          pressedKey.key, keyModifiers.shift);

            if (!typedText.empty())
            {
                keyBench.typedThisFrame += typedText;
                keyBench.keysNow.push_back(
                    antwika::ui::Key::Character);
            }

            const auto moving = getMovingKey(pressedKey.key, keyModifiers.shift);

            if (moving.has_value())
            {
                keyBench.keysNow.push_back(*moving);
            }

            if (keyModifiers.control && pressedKey.key == input::Key::A)
            {
                keyBench.keysNow.push_back(antwika::ui::Key::SelectAll);
            }

            if (keyModifiers.control && pressedKey.key == input::Key::C)
            {
                keyBench.keysNow.push_back(antwika::ui::Key::Copy);
            }

            if (keyModifiers.control && pressedKey.key == input::Key::X)
            {
                keyBench.keysNow.push_back(antwika::ui::Key::Cut);
            }

            if (pressedKey.key == input::Key::Enter
                && focusedField == FocusedField::PlanBody)
            {
                keyBench.keysNow.push_back(antwika::ui::Key::Activate);

                return true;
            }

            if (pressedKey.key == input::Key::Enter
                || pressedKey.key == input::Key::Escape)
            {
                if (focusedField == FocusedField::ComponentValue
                    && pressedKey.key == input::Key::Enter)
                {
                    commitComponentEdit();
                }
                else if (focusedField == FocusedField::ComponentValue)
                {
                    worldView.characterTool().endValueEdit();
                }
                else if (focusedField == FocusedField::MarkerAxis
                         && pressedKey.key == input::Key::Enter)
                {
                    commitMarkerEdit();
                }
                else if (focusedField == FocusedField::MarkerAxis)
                {
                    markerPick.editingAxis.reset();
                    markerPick.pendingAxisText.clear();
                }
                else if (focusedField == FocusedField::EntityAxis
                         && pressedKey.key == input::Key::Enter)
                {
                    commitEntityEdit();
                }
                else if (focusedField == FocusedField::EntityAxis)
                {
                    entityPick.editingAxis.reset();
                    entityPick.pendingAxisText.clear();
                }

                focusedField = FocusedField::Nothing;
            }

            return true;
        }

        return false;
    }

}
