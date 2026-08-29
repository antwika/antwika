#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/app/TickDebt.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/gfx/IGfxBackend.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/gfx/ViewportRenderer.hpp>
#include <antwika/input/IInputBackend.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputState.hpp>
#include <antwika/input/KeyModifiers.hpp>
#include <antwika/input/Position.hpp>
#include <antwika/light/ActiveLight.hpp>
#include <antwika/log/ILogger.hpp>
#include <antwika/map/MapFile.hpp>
#include <antwika/render/AtlasSheets.hpp>
#include <antwika/render/LightPasses.hpp>
#include <antwika/render/ScenePass.hpp>
#include <antwika/render/Sprites.hpp>
#include <antwika/render/WorldShader.hpp>
#include <antwika/render/WorldMeshes.hpp>
#include <antwika/time/SystemClock.hpp>
#include <antwika/ui/Context.hpp>
#include <antwika/ui/Frame.hpp>
#include <antwika/voxelmap/Voxel.hpp>

#include "antwika/editor/LayerEdit.hpp"
#include "antwika/editor/Preferences.hpp"
#include "antwika/editor/WorldCamera.hpp"
#include "antwika/editor/view/IEditSteps.hpp"
#include "antwika/editor/view/INotices.hpp"
#include "antwika/editor/view/IEditorView.hpp"
#include "antwika/editor/view/ViewContext.hpp"
#include "antwika/editor/view/WorldSprites.hpp"
#include "antwika/editor/editor/state/AssignMode.hpp"
#include "antwika/editor/editor/state/CanvasRest.hpp"
#include "antwika/editor/editor/state/Dialogs.hpp"
#include "antwika/editor/editor/state/FocusedField.hpp"
#include "antwika/editor/editor/state/FrameMeters.hpp"
#include "antwika/editor/editor/state/PointerTrack.hpp"
#include "antwika/editor/editor/state/CharacterTool.hpp"
#include "antwika/editor/editor/state/GizmoSet.hpp"
#include "antwika/editor/editor/state/KeyBench.hpp"
#include "antwika/editor/editor/state/EntityList.hpp"
#include "antwika/editor/editor/state/EntityPick.hpp"
#include "antwika/editor/editor/state/MarkerPick.hpp"
#include "antwika/editor/editor/state/OverlayCache.hpp"
#include "antwika/editor/editor/state/RemeshDebt.hpp"
#include "antwika/editor/editor/state/ViewChoice.hpp"
#include "antwika/editor/editor/state/WorldPaint.hpp"
#include "antwika/editor/editor/state/SheetStroke.hpp"
#include "antwika/editor/editor/state/SheetView.hpp"
#include "antwika/editor/editor/state/WorldEdit.hpp"

#include "antwika/editor/editor/state/StampTool.hpp"
#include "antwika/editor/editor/state/TilePreview.hpp"
#include "antwika/editor/editor/state/TransitionPick.hpp"
#include "antwika/editor/editor/state/StatusMessage.hpp"
#include "antwika/editor/editor/FileChooser.hpp"
#include "antwika/editor/editor/CameraRig.hpp"
#include "antwika/editor/editor/EditorDocument.hpp"
#include "antwika/editor/editor/GameModule.hpp"
#include "antwika/editor/editor/InkPanel.hpp"
#include "antwika/editor/editor/PlaySession.hpp"
#include "antwika/editor/editor/SimulationSteps.hpp"
#include "antwika/editor/plan/PlanBoard.hpp"
#include "antwika/editor/plan/PlanFile.hpp"
#include "antwika/editor/ui/AtlasSheetsView.hpp"
#include "antwika/editor/ui/AtlasView.hpp"
#include "antwika/editor/ui/WorldView.hpp"

#include "antwika/editor/ui/CharacterSheetView.hpp"
#include "antwika/editor/ui/EditorBindings.hpp"
#include "antwika/editor/ui/GizmoView.hpp"
#include "antwika/editor/ui/IconsView.hpp"
#include "antwika/editor/ui/MenuBar.hpp"
#include "antwika/editor/ui/PlanView.hpp"
#include "antwika/editor/ui/ToolButtonRow.hpp"
#include "antwika/editor/ui/ToolGroupMembers.hpp"
#include "antwika/editor/ui/ToolPanel.hpp"
#include "antwika/editor/ui/ToolPlacement.hpp"
#include "antwika/editor/ui/ToolPlacementRow.hpp"

namespace antwika::editor::widget_catalog
{

    struct Catalog;

}

namespace antwika::editor::fakes
{

    struct EditorProbe;

}

namespace antwika::editor
{

    inline constexpr std::string_view kAppName = "antwika_editor";

    inline constexpr float kCameraFollowLerp = 0.12F;

    class Editor final : public IEditSteps, public INotices
    {
    public:
        Editor(
            log::ILogger &logger,
            gfx::IGfxBackend &backend,
            input::IInputBackend &inputs,
            std::string mapPathGiven,
            bool playOnlyGiven = false,
            std::string planPathGiven =
                std::string(kDefaultPlanPath));

        void run();

        [[nodiscard]] component::Position playerStandsAt() const;

        [[nodiscard]] bool isPlaying() const noexcept
        {
            return play.playing;
        }

        [[nodiscard]] View getActiveView() const noexcept
        {
            return viewChoice.activeView;
        }

        [[nodiscard]] const CharacterSheetView &getCharacterView()
            const noexcept
        {
            return characterView;
        }

    private:
        log::ILogger &logger;
        bool playOnly = false;
        gfx::IGfxBackend &backend;
        input::IInputBackend &inputs;
        std::unique_ptr<gfx::IWindow> window;
        gfx::ViewportRenderer viewportRenderer;
        render::WorldShader worldShader;
        render::Sprites sprites;

        EditorDocument document;

        render::WorldMeshes worldMeshes;

        std::uint32_t tick = 0;

        render::AtlasSheets atlasSheets;

        AtlasSheetsView atlasView;

        WorldView worldView;

        IconsView iconsView;

        GizmoSet gizmos;

        GizmoView gizmoView;

        CharacterSheetView characterView;

        render::CharacterSkins characterSkins;

        render::ScenePass scenePass;

        PlaySession play;

        CameraRig cameraRig;

        PlanView plan;

        Preferences preferences;

        bool turningPlayer = false;
        bool running = true;
        input::InputState inputState;

        Dialogs dialogs;

        FileChooser fileChooser;

        StatusMessage statusMessageNotice;

        PointerTrack pointer;

        TransitionPick transition;

        TilePreview preview;

        RemeshDebt remesh;

        KeyBench keyBench;

        SheetView sheetView;

        SheetStroke stroke;

        ViewChoice viewChoice;

        SimulationSteps simulation{document, tick};

        InkPanel inkPanel{
            document,
            atlasSheets,
            characterView,
            characterSkins,
            viewportRenderer,
            *this};

        std::size_t chosenLayer = map::kBaseLayer;
        AssignMode assignMode;
        MarkerPick markerPick;
        EntityPick entityPick;
        EntityList entityList;
        std::optional<widget::WidgetId> slidingWidget;
        FocusedField focusedField = FocusedField::Nothing;

        render::LightPasses lightPasses;

        FrameMeters meters;

        time::SystemClock clockSource;
        app::TickDebt tickDebt{clockSource};

        [[nodiscard]] ViewContext viewContextNow() noexcept;

        [[nodiscard]] IEditorView *viewNow() noexcept;

        [[nodiscard]] bool isWorldShown() const noexcept;

        /**
         * @brief The world stands in a panel of its own while the
         * editor is up, and fills the window while the game plays.
         */
        [[nodiscard]] bool isWorldPanelShown() const noexcept;

        [[nodiscard]] bool pollWindow();
        void pollInputs();

        void loadMapOrBuiltIn();
        void loadPlan(std::string planPathGiven);
        void openSheets();
        void openGizmoSheet();
        void openPasses();
        void aimOpeningCamera();
        void beginPlay();
        void restoreProgress();
        void keepMapForPlay();
        void restoreMapAfterPlay();

        [[nodiscard]] voxel::Voxels visibleCells();
        void rebuildWorld() override;
        [[nodiscard]] map::Placement startingPlacement();
        void standPlayer();
        void moveCamera();
        void aimPlayCamera();

        void turnPlayer(float byYaw, float byPitch);
        void rightTaken(input::Position position);
        void saveCurrentMap();
        bool loadCurrentMap();
        void startNewMap();
        void confirmFileDialog();
        [[nodiscard]] Snapshot snapshot();
        void pushUndo() override;
        void applyStep(Snapshot stepSnapshot);

        void pressTool(ToolButton whichButton);

        void toggleFreeLook();

        [[nodiscard]] bool isToolButtonActive(
            ToolButton whichButton) const;
        void flipEdgeToggle(EdgeToggle whichToggle);
        void deriveRulesFromShapes();

        [[nodiscard]] std::optional<geometry::GridCell> cellUnderPointer();
        void drawColorPicker();
        using MenuFlag = bool &(*)(Editor &);

        [[nodiscard]] static MenuFlag getToggledFlag(MenuItem item);

        void takePreferences(const Preferences &shownPreferences);

        [[nodiscard]] Preferences getPreferencesAsShown() const;
        void onMenuItem(MenuItem item);
        [[nodiscard]] bool isChecked(MenuItem item);
        [[nodiscard]] std::string statusText();

        [[nodiscard]] static const widget_catalog::Catalog &getWidgetCatalog();

        [[nodiscard]] std::string_view hintFor(
            widget::WidgetId whichWidget) const;

        void drawToolHint(const ui::Frame &frame);

        void updateCanvasHover(const ui::Frame &frame);

        void drawCanvasHint();

        void finishView(
            const ui::Frame &frame,
            std::chrono::time_point<std::chrono::system_clock>
                startedAt);

        void placeStartOrExit(
            voxel::VoxelPosition position, input::MouseButton button);

        [[nodiscard]] std::string getCharacterSheetPath(
            std::size_t position) const;

        void loadCharacterSkins();

        void saveCharacterSkins();

        void pressCharacter(
            voxel::VoxelPosition position, input::MouseButton button);

        void ensurePlayerCharacter();

        void spawnCharacters();

        void spawnItems();

        void playApart();

        void layoutWorldRail(ui::Context &context);

        void pickedVariant(tilemap::Tile tile);
        [[nodiscard]] bool consumeAssignClick(tilemap::Tile tile) override;
        [[nodiscard]] bool blockedAsVariant() override;
        [[nodiscard]] std::uint8_t variantWeightOf(
            tilemap::Tile tile) const;
        void layoutDecorRail(ui::Context &context);
        void ensureDecor();
        [[nodiscard]] std::optional<tilemap::Tile> freeTileSlot(
            tilemap::Atlas atlas);
        void copyTilePixels(tilemap::Tile fromTile, tilemap::Tile toTile);
        void wipeTile(tilemap::Tile tile) override;
        void layoutVariantRail(ui::Context &context);
        void layoutSpanRows(
            ui::Context &context, const decor::DecorTile &decor);
        void layoutFlipRail(ui::Context &context);
        void layoutTransitionRail(ui::Context &context);
        void pressedOnSheets(
            const input::PointerButtonPressed &downPressed);
        void pressMarker(
            voxel::VoxelPosition position, input::MouseButton button);
        [[nodiscard]] std::vector<light::ActiveLight> currentLights();
        void onSteppedWorld();
        void clearAssignModes();
        [[nodiscard]] widget::WidgetId getWidgetForField(
            FocusedField focusedField) const;
        void showStatus(
            const std::string &text,
            bool warns = false,
            std::uint32_t durationTicks = kNoticeTicks) override;
        [[nodiscard]] bool pickedTransition(tilemap::Tile tile);
        [[nodiscard]] bool shouldAdvanceTileAnimation() const;
        void uploadAtlases(bool force);
        [[nodiscard]] bool nudgeSpan(int acrossStep, int downStep);
        [[nodiscard]] bool characterWidgets(
            const ui::Interactions &interactions);

        void layoutComponentSection(
            ui::Context &context, std::size_t chosenCharacter);

        [[nodiscard]] bool componentWidgets(
            const ui::Interactions &interactions);

        void carryComponentScroll(const ui::Interactions &interactions);

        [[nodiscard]] bool isInspectorHovered() const;

        void commitComponentEdit();

        [[nodiscard]] bool isMarkerSectionShown() const;

        void layoutMarkerSection(ui::Context &context);

        void commitMarkerEdit();

        void dropMarkerPick();

        void pressSelect(
            voxel::VoxelPosition position, input::MouseButton button);

        [[nodiscard]] std::optional<EntityPick> findEntityAt(
            voxel::VoxelPosition position) const;

        void keepEntityStep();

        bool moveEntityTo(
            voxel::VoxelPosition nextPosition, bool snapsToGround);

        void carryEntity();

        void removeEntityPick();

        void dropEntityPick();

        void commitEntityEdit();

        [[nodiscard]] bool isEntitySectionShown() const;

        void layoutEntitySection(ui::Context &context);

        void rebuildEntityRows();

        void layoutEntityListPanel(ui::Context &context);

        [[nodiscard]] bool pressEntityRow(std::size_t place);

        void carryEntityListScroll(const ui::Interactions &interactions);

        [[nodiscard]] bool isEntityListHovered() const;

        void layoutCharacterChooser(ui::Context &context);

        void endSliderDrag();

        [[nodiscard]] bool beginEdgeDrag(
            const ui::Interactions &interactions);

        void endEdgeDrag();

        [[nodiscard]] std::uint32_t panelWidthOf(
            std::uint32_t PanelSizes::*extent,
            std::uint32_t restingWidth) const;

        [[nodiscard]] float getRailWidthOnCanvas() const;

        void duplicateTile(
            geometry::GridCell fromCell, geometry::GridCell toCell) override;

        [[nodiscard]] bool beginSliderDrag(
            const ui::Interactions &interactions);

        void undo();

        void redo();

        void growChunk();

        void settleHistory(
            std::optional<Snapshot> stepSnapshot,
            const std::string &doneLabel,
            const std::string &nothing);

        void pathTo(input::Position position);

        void standPlayerAt(std::int32_t x, std::int32_t z);

        void takeExit();

        [[nodiscard]] std::string getProgressPath() const;

        void savePlayerProgress();

        [[nodiscard]] input::KeyModifiers getHeldModifiers() const noexcept;

        [[nodiscard]] bool matchesChord(
            Action action, input::Key key) const;

        [[nodiscard]] bool matchesChordWithShift(
            Action action, input::Key key) const;

        void applyWalkKey(input::Key key, bool down);

        void applyRunKey(input::Key key, bool down);

        [[nodiscard]] bool consumeBindingsKey(
            const input::KeyPressed &pressedKey);

        [[nodiscard]] std::string getChordsPath() const;

        [[nodiscard]] bool layoutModals(ui::Context &context);

        [[nodiscard]] bool consumeModalWidgets(
            const ui::Interactions &interactions);

        [[nodiscard]] bool consumeTextInput(
            const input::KeyPressed &pressedKey);
        void simulate();

#ifdef ANTWIKA_GAME_SHARED
        void reloadGameModule();
#endif

        void recordFrameWork(
            std::chrono::time_point<std::chrono::system_clock>
                startedAt);

        [[nodiscard]] ui::Frame layoutUi(
            bool pressed, bool buttonHeld);
        void layoutSidebar(ui::Context &context);

        void onScrolled(const input::PointerScrolled &rolledScrolled);
        void onPointerMoved(const input::PointerMoved &movedEvent);
        void onPointerReleased(
            const input::PointerButtonReleased &upReleased);

        void onPointerPressed(
            const input::PointerButtonPressed &downPressed);
        [[nodiscard]] bool consumeWidgets(
            const ui::Interactions &interactions);

        void onKeyReleased(const input::KeyReleased &releasedEvent);
        void onKeyPressed(const input::KeyPressed &pressedKey);

        void frame(
            std::chrono::time_point<std::chrono::system_clock>
                startedAt);

        void setView(View nextView);

        friend struct fakes::EditorProbe;
    };

}
