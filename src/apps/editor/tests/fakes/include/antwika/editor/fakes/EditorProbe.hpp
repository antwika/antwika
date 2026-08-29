#pragma once

#include <chrono>
#include <cstddef>
#include <string>

#include <antwika/ecs/World.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/ui/Interactions.hpp>

#include "antwika/editor/Editor.hpp"
#include "antwika/editor/ui/WidgetCatalog.hpp"

namespace antwika::editor::fakes
{

    struct EditorProbe final
    {
        explicit EditorProbe(Editor &editorGiven)
            : document(editorGiven.document),
              stroke(editorGiven.stroke),
              pointer(editorGiven.pointer),
              chosenLayer(editorGiven.chosenLayer),
              editor(editorGiven)
        {
        }

        EditorDocument &document;

        SheetStroke &stroke;

        PointerTrack &pointer;

        std::size_t &chosenLayer;

        [[nodiscard]] static const widget_catalog::Catalog &getCatalog()
        {
            return Editor::getWidgetCatalog();
        }

        [[nodiscard]] bool beginSliderDrag(
            const ui::Interactions &interactions)
        {
            return editor.beginSliderDrag(interactions);
        }

        [[nodiscard]] bool beginEdgeDrag(
            const ui::Interactions &interactions)
        {
            return editor.beginEdgeDrag(interactions);
        }

        void endEdgeDrag()
        {
            editor.endEdgeDrag();
        }

        void carryComponentScroll(const ui::Interactions &interactions)
        {
            editor.carryComponentScroll(interactions);
        }

        [[nodiscard]] bool isInspectorHovered() const
        {
            return editor.isInspectorHovered();
        }

        void onScrolled(const input::PointerScrolled &rolledScrolled)
        {
            editor.onScrolled(rolledScrolled);
        }

        [[nodiscard]] CharacterTool &characterTool()
        {
            return editor.worldView.characterTool();
        }

        [[nodiscard]] WorldView &worldView()
        {
            return editor.worldView;
        }

        [[nodiscard]] MarkerPick &markerPick()
        {
            return editor.markerPick;
        }

        [[nodiscard]] Preferences &preferences()
        {
            return editor.preferences;
        }

        [[nodiscard]] bool isMarkerSectionShown() const
        {
            return editor.isMarkerSectionShown();
        }

        void pressMarker(
            const voxel::VoxelPosition position,
            const input::MouseButton button)
        {
            editor.pressMarker(position, button);
        }

        void pressSelect(
            const voxel::VoxelPosition position,
            const input::MouseButton button)
        {
            editor.pressSelect(position, button);
        }

        [[nodiscard]] EntityPick &entityPick()
        {
            return editor.entityPick;
        }

        [[nodiscard]] bool isEntitySectionShown() const
        {
            return editor.isEntitySectionShown();
        }

        [[nodiscard]] EntityList &entityList()
        {
            return editor.entityList;
        }

        [[nodiscard]] CameraRig &cameraRig()
        {
            return editor.cameraRig;
        }

        bool pressEntityRow(const std::size_t place)
        {
            return editor.pressEntityRow(place);
        }

        void carryEntityListScroll(const ui::Interactions &interactions)
        {
            editor.carryEntityListScroll(interactions);
        }

        [[nodiscard]] bool isEntityListHovered() const
        {
            return editor.isEntityListHovered();
        }

        void spawnCharacters()
        {
            editor.spawnCharacters();
        }

        void removeEntityPick()
        {
            editor.removeEntityPick();
        }

        bool moveEntityTo(
            const voxel::VoxelPosition nextPosition,
            const bool snapsToGround)
        {
            return editor.moveEntityTo(nextPosition, snapsToGround);
        }

        void turnPlayer(const float byYaw, const float byPitch)
        {
            editor.turnPlayer(byYaw, byPitch);
        }

        void pressTool(const ToolButton whichButton)
        {
            editor.pressTool(whichButton);
        }

        [[nodiscard]] bool isToolButtonActive(
            const ToolButton whichButton) const
        {
            return editor.isToolButtonActive(whichButton);
        }

        void onMenuItem(const MenuItem item)
        {
            editor.onMenuItem(item);
        }

        [[nodiscard]] bool isChecked(const MenuItem item)
        {
            return editor.isChecked(item);
        }

        [[nodiscard]] FocusedField getFocusedField() const
        {
            return editor.focusedField;
        }

        [[nodiscard]] ecs::World &playedWorld()
        {
            return editor.play.game->getWorld();
        }

        [[nodiscard]] ecs::Entity getEye() const
        {
            return editor.play.game->getEye();
        }

        bool consumeWidgets(const ui::Interactions &interactions)
        {
            return editor.consumeWidgets(interactions);
        }

        bool consumeTextInput(const input::KeyPressed &pressedKey)
        {
            return editor.consumeTextInput(pressedKey);
        }

        void carryEdit(
            const widget::WidgetId fieldWidget, const std::string &text)
        {
            widget_catalog::carryFamilyEdit(
                Editor::getWidgetCatalog(), editor, fieldWidget, text);
        }

        void keyPressed(const input::KeyPressed &pressedKey)
        {
            editor.onKeyPressed(pressedKey);
        }

        void pumpFrame()
        {
            editor.frame(std::chrono::system_clock::now());
        }

        [[nodiscard]] ui::Frame layoutUi()
        {
            return editor.layoutUi(false, false);
        }

        [[nodiscard]] SheetView &sheetView()
        {
            return editor.sheetView;
        }

        [[nodiscard]] ViewChoice &viewChoice()
        {
            return editor.viewChoice;
        }

        [[nodiscard]] gfx::Size getWindowSize() const
        {
            return editor.viewportRenderer.getWindowSize();
        }

        void pointerPressed(
            const input::PointerButtonPressed &downPressed)
        {
            editor.onPointerPressed(downPressed);
        }

        void pointerReleased(
            const input::PointerButtonReleased &upReleased)
        {
            editor.onPointerReleased(upReleased);
        }

        void undo()
        {
            editor.undo();
        }

        void pushUndo()
        {
            editor.pushUndo();
        }

    private:
        Editor &editor;
    };

}
