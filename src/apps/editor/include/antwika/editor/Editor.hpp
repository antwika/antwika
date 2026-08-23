#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/app/FramePacing.hpp>
#include <antwika/app/TickDebt.hpp>
#include <antwika/camera/FlyCamera.hpp>
#include <antwika/character/Character.hpp>
#include <antwika/component/AnimationState.hpp>
#include <antwika/component/Item.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/IGfxBackend.hpp>
#include <antwika/gfx/IMesh.hpp>
#include <antwika/gfx/IRenderTarget.hpp>
#include <antwika/gfx/IShader.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/gfx/ViewportRenderer.hpp>
#include <antwika/input/ActionMap.hpp>
#include <antwika/input/DirectionKeys.hpp>
#include <antwika/input/IInputBackend.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputState.hpp>
#include <antwika/input/KeyModifiers.hpp>
#include <antwika/input/Position.hpp>
#include <antwika/light/ActiveLight.hpp>
#include <antwika/log/ILogger.hpp>
#include <antwika/map/EditHistory.hpp>
#include <antwika/map/MapFile.hpp>
#include <antwika/map/PlayerProgress.hpp>
#include <antwika/render/AtlasSheets.hpp>
#include <antwika/render/LightPasses.hpp>
#include <antwika/render/ScenePass.hpp>
#include <antwika/render/Sprites.hpp>
#include <antwika/render/WorldShader.hpp>
#include <antwika/render/WorldMeshes.hpp>
#include <antwika/solver/VoxelWeave.hpp>
#include <antwika/time/FrameRate.hpp>
#include <antwika/time/SystemClock.hpp>
#include <antwika/ui/Context.hpp>
#include <antwika/ui/DoubleClick.hpp>
#include <antwika/ui/Frame.hpp>
#include <antwika/ui/HoverHint.hpp>
#include <antwika/ui/Keyboard.hpp>
#include <antwika/voxelmap/Voxel.hpp>
#include <antwika/voxelmap/VoxelPick.hpp>
#include <antwika/collision/Collision.hpp>
#include <antwika/worldgen/ChunkShape.hpp>

#include "antwika/editor/editor/state/AssignMode.hpp"
#include "antwika/editor/editor/state/CanvasRest.hpp"
#include "antwika/editor/editor/state/Dialogs.hpp"
#include "antwika/editor/editor/state/Caption.hpp"
#include "antwika/editor/editor/state/FocusedField.hpp"
#include "antwika/editor/editor/state/FrameMeters.hpp"
#include "antwika/editor/editor/state/PointerTrack.hpp"
#include "antwika/editor/editor/state/InkPicker.hpp"
#include "antwika/editor/editor/state/StatusMessage.hpp"
#include "antwika/editor/editor/FileDialog.hpp"
#include "antwika/editor/editor/GameModule.hpp"
#include "antwika/editor/plan/PlanBoard.hpp"
#include "antwika/editor/plan/PlanFile.hpp"
#include "antwika/editor/ui/AtlasView.hpp"
#include "antwika/editor/ui/CharacterSheetView.hpp"
#include "antwika/editor/ui/ColorPicker.hpp"
#include "antwika/editor/ui/EditorBindings.hpp"
#include "antwika/editor/ui/IconsView.hpp"
#include "antwika/editor/ui/MenuBar.hpp"
#include "antwika/editor/ui/PlanView.hpp"
#include "antwika/editor/ui/ToolButtonRow.hpp"
#include "antwika/editor/ui/ToolPanel.hpp"
#include "antwika/editor/ui/ToolPlacement.hpp"
#include "antwika/editor/ui/ToolPlacementRow.hpp"
#include "antwika/editor/ui/ToolToggles.hpp"
#include "antwika/rules/Gates.hpp"
#include "antwika/system/HealthSystem.hpp"
#include "antwika/system/OrientationSystem.hpp"
#include "antwika/system/PatrolSystem.hpp"

namespace antwika::editor
{

    inline constexpr std::string_view kAppName = "antwika_editor";


    inline constexpr float kCameraFollowLerp = 0.12F;

    class Editor final
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

    private:
        log::ILogger &logger;
        std::string mapPath;
        std::string startMapPath;
        bool playOnly = false;
        bool titleScreenUp = false;
        gfx::IGfxBackend &backend;
        input::IInputBackend &inputs;
        std::unique_ptr<gfx::IWindow> window;
        gfx::ViewportRenderer viewportRenderer;
        render::WorldShader worldShader;
        render::Sprites sprites;

        map::Map map;
        solver::CornerSeams cornerJoining = solver::CornerSeams::Ignored;

        render::WorldMeshes worldMeshes;

        std::vector<std::vector<voxel::VoxelPosition>> patrolPositions;
        std::int32_t editLevel = 0;
        bool hideAboveLevel = false;
        bool playing = false;

        bool overlayStale = true;
        std::vector<voxelmap::LineSegment> gridLines;
        std::vector<voxelmap::LineSegment> topLines;
        std::vector<solver::FaceSeam> seamsAboveLevel;
        std::vector<solver::FaceSeam> seamsAtLevel;
        std::uint32_t tick = 0;
        map::EditHistory history;

        render::AtlasSheets atlasSheets;

        IconsView iconsView;

        CharacterSheetView characterView;

        render::ScenePass scenePass;

        ecs::World world;

        GameModule game;

        map::CameraView cameraView;

        float viewHeight = 0.0F;
        bool freeLook = false;
        bool cameraFollows = true;

        bool panning = false;
        std::optional<gfx::Vec3> panGripPosition;
        bool orbiting = false;
        std::optional<input::Position> orbitFromPosition;

        bool descendHeld = false;
        bool ascendHeld = false;


        map::View activeView = map::View::World;

        map::View viewBeforePlay = map::View::World;

        PlanView plan;

        map::Tool tool = map::Tool::Brush;
        bool lighting = true;
        bool showRuleLines = true;
        map::Paint paintMode = map::Paint::Brush;
        voxel::Kind brushKind = voxel::Kind::Normal;
        voxel::Facing rampFacing = voxel::Facing::Any;
        bool grid = true;
        bool showPlacementGhost = true;
        bool lampSight = true;
        bool lowerSight = true;
        bool lowerLight = true;

        bool turningPlayer = false;
        bool running = true;
        input::InputState inputState;

        Dialogs dialogs;
        bool dirty = false;

        StatusMessage statusMessageNotice;

        std::uint64_t growSeed = 0;

        worldgen::ChunkShape growShape{};

        std::vector<voxel::VoxelPosition> growTroublePositions;

        PointerTrack pointer;

        std::uint32_t lastWheelNudgeTick = 0;
        bool remeshAfterNudge = false;

        bool remeshPending = false;

        std::optional<voxel::VoxelPosition> shapeFromPosition;
        std::optional<light::Lamp> draggedLamp;

        voxel::Voxels stampVoxels;
        std::optional<voxel::VoxelPosition> stampFromPosition;

        std::optional<std::size_t> figurePicked;
        bool figurePlaced = false;
        std::string pendingFigureLine;
        Caption caption;

        std::optional<std::size_t> platePicked;
        std::optional<voxel::VoxelPosition> lastPlateStoodOnPosition;

        KeyBindings bindings = defaultChords();
        input::ActionMap actions = actionMapFrom(bindings);
        bool keysOpen = false;
        std::optional<Action> rebindingAction;
        std::string typedThisFrame;
        std::vector<ui::Key> keysNow;

        InkPicker inkPicker;

        std::size_t chosenLayer = map::kBaseLayer;
        AssignMode assignMode;
        std::optional<tilemap::Tile> transitFromTile;
        std::optional<tilemap::Tile> transitToTile;
        std::optional<std::size_t> transitionPicked;

        std::optional<widget::WidgetId> slidingWidget;
        FocusedField focusedField = FocusedField::Nothing;

        bool previewAuto = true;
        std::uint32_t previewSeed = 0;
        std::optional<tilemap::Tile> previewForTile;
        std::size_t previewLayer = map::kBaseLayer;
        std::optional<std::vector<std::optional<tilemap::Tile>>> previewTiles;

        std::optional<geometry::GridCell> dragFromCell;
        std::optional<gfx::PointF> dragFromPoint;
        std::optional<gfx::PointF> doubleClickAtPoint;
        std::optional<tilemap::Tile> selectedTile;
        std::optional<EdgeSelection> selectedEdges;
        bool strokeActive = false;
        bool strokeErases = false;
        std::optional<geometry::GridCell> lineFromCell;
        std::optional<geometry::GridCell> brushAtCell;
        float gridZoom = 1.0F;
        gfx::PointF gridPanPoint{};
        std::optional<gfx::RectF> sheetRect;
        std::optional<gfx::RectF> canvasRect;

        std::optional<input::MouseButton> dragPaintButton;
        std::optional<voxel::VoxelPosition> lastPaintedPosition;

        render::LightPasses lightPasses;

        FrameMeters meters;

        time::SystemClock clockSource;
        app::TickDebt tickDebt{clockSource};

        [[nodiscard]] bool pollWindow();
        void pollInputs();

        [[nodiscard]] voxel::Voxels visibleCells();
        void rebuildDecorMesh();
        void rebuildWorld();
        [[nodiscard]] map::Placement startingPlacement();
        void standPlayer();
        [[nodiscard]] gfx::Mat4 worldRotation();
        [[nodiscard]] gfx::Camera3D worldCamera();
        void moveCamera();
        void aimPlayCamera();
        void orbitCamera(float byYaw, float byPitch);

        void turnPlayer(float byYaw, float byPitch);
        void rightTaken(input::Position position);
        void saveCurrentMap();
        bool loadCurrentMap();
        void startNewMap();
        void listFolder(const std::string &folder);
        void openFileDialog(bool forSave);
        void confirmFileDialog();
        void cancelFileDialog();
        [[nodiscard]] map::Snapshot snapshot();
        void pushUndo();
        void applyStep(map::Snapshot stepSnapshot);

        void pressTool(ToolButton whichButton);

        [[nodiscard]] bool toolButtonActive(
            ToolButton whichButton) const;
        [[nodiscard]] tile::TileRules &activeRules();
        [[nodiscard]] bool isDecorLayer();
        void commitFloatingPatch();
        void mirrorSelection();
        void flipEdgeToggle(EdgeToggle whichToggle);
        void deriveRulesFromShapes();

        [[nodiscard]] bool selectionIsForbidden();
        [[nodiscard]] bool selectionAllowsBoundary();
        [[nodiscard]] bool selectionAllows(tilemap::Tile neighbourTile);
        [[nodiscard]] gfx::RectF sheetClipRect();
        [[nodiscard]] gfx::RectF gridRect();
        [[nodiscard]] gfx::RectF frameRect();
        [[nodiscard]] std::optional<geometry::GridCell> cellUnderPointer();
        void drawColorPicker();
        using MenuFlag = bool Editor::*;

        [[nodiscard]] static MenuFlag toggledFlag(MenuItem item);
        void onMenuItem(MenuItem item);
        [[nodiscard]] bool isChecked(MenuItem item);
        [[nodiscard]] std::string statusText();

        [[nodiscard]] std::string_view hintFor(
            widget::WidgetId whichWidget) const;

        void drawToolHint(const ui::Frame &frame);

        void updateCanvasHover(const ui::Frame &frame);

        void drawCanvasHint();

        void finishView(
            const ui::Frame &frame,
            std::chrono::time_point<std::chrono::system_clock>
                startedAt);

        [[nodiscard]] std::vector<voxel::VoxelPosition> shapedCubes(
            voxel::VoxelPosition fromPosition,
            voxel::VoxelPosition toPosition) const;

        bool beginShape(
            voxel::VoxelPosition position, input::MouseButton button);

        void finishShape(input::MouseButton button);

        void placeStartOrExit(
            voxel::VoxelPosition position, input::MouseButton button);

        void pressStamp(
            voxel::VoxelPosition position, input::MouseButton button);

        void finishStamp(input::MouseButton button);

        [[nodiscard]] std::vector<voxel::VoxelPosition> stampGhost(
            voxel::VoxelPosition positionCell) const;

        [[nodiscard]] std::string characterSheetPath(
            std::size_t position) const;

        void loadCharacterSkins();

        void saveCharacterSkins();

        void pressFigure(
            voxel::VoxelPosition position, input::MouseButton button);

        void ensurePlayerInRoster();

        void spawnRoster();

        void spawnItems();

        void playApart();

        void drawHealthBars(const gfx::Mat4 &clipMatrix);

        void drawPointMark(
            const gfx::Mat4 &clipMatrix,
            gfx::Vec3 position,
            gfx::Color markColor);

        void drawSightPoints(const gfx::Mat4 &clipMatrix);

        void consumeItem(component::ItemKind kind);
        void sayConsumeReport();
        void sayDialogueLine();

        void interact();

        void pressPlate(
            voxel::VoxelPosition position, input::MouseButton button);

        void onSteppedPlates(voxel::VoxelPosition standsOnPosition);

        void layoutWorldRail(ui::Context &context);

        [[nodiscard]] bool variantWidgets(
            const ui::Interactions &interactions);
        void pickedVariant(tilemap::Tile tile);
        [[nodiscard]] bool handleAssignClick(tilemap::Tile tile);
        [[nodiscard]] bool blockedAsVariant();
        [[nodiscard]] std::uint8_t variantWeightOf(
            tilemap::Tile tile) const;
        void layoutDecorRail(ui::Context &context);
        void ensureDecor();
        [[nodiscard]] tilemap::Tile editedTile();
        [[nodiscard]] std::optional<tilemap::Tile> freeTileSlot(
            tilemap::Atlas atlas);
        void copyTilePixels(tilemap::Tile fromTile, tilemap::Tile toTile);
        void wipeTile(tilemap::Tile tile);
        void layoutVariantRail(ui::Context &context);
        void layoutSpanRows(
            ui::Context &context, const decor::DecorTile &decor);
        void layoutFlipRail(ui::Context &context);
        void layoutTransitionRail(ui::Context &context);
        [[nodiscard]] bool paintedOnAtlasPixel();
        void pressedOnSheets(
            const input::PointerButtonPressed &downPressed);
        void pressGate(
            voxel::VoxelPosition position, input::MouseButton button);
        [[nodiscard]] std::vector<light::ActiveLight> currentLights();
        void onSteppedGates(
            voxel::VoxelPosition standsInPosition,
            voxel::VoxelPosition standsOnPosition);
        void onSteppedKeys(
            voxel::VoxelPosition standsInPosition,
            voxel::VoxelPosition standsOnPosition);
        void onSteppedCheckpoints(voxel::VoxelPosition standsOnPosition);
        void onSteppedDoors(voxel::VoxelPosition standsInPosition);
        void onSteppedWorld(gfx::Vec3 walkerPosition);
        [[nodiscard]] bool tryUnlockExit();
        void resetGates();
        void clearAssignModes();
        [[nodiscard]] static widget::WidgetId widgetForField(
            FocusedField focusedField);
        void sayCaption(
            const std::string &name,
            const std::string &line,
            std::optional<std::size_t> speaker = std::nullopt);
        void showStatus(
            const std::string &text,
            bool warns = false,
            std::uint32_t durationTicks = 240);
        [[nodiscard]] bool transitionWidgets(
            const ui::Interactions &interactions);
        [[nodiscard]] bool pickedTransition(tilemap::Tile tile);
        [[nodiscard]] bool blockedAsTransitionSlot();
        [[nodiscard]] bool flipWidgets(
            const ui::Interactions &interactions);
        [[nodiscard]] bool shouldAdvanceTileAnimation() const;
        void uploadAtlases(bool force);
        [[nodiscard]] bool spanWidgets(
            const ui::Interactions &interactions);
        [[nodiscard]] bool figureRosterWidgets(
            const ui::Interactions &interactions);

        void layoutFigureChooser(ui::Context &context);

        [[nodiscard]] bool handlePaletteWidgets(
            const ui::Interactions &interactions);

        [[nodiscard]] bool handlePickerPress(
            const input::PointerButtonPressed &downPressed);

        void endSliderDrag();

        void drawSprite(
            const gfx::Camera3D &camera,
            const gfx::Mat4 &modelMatrix,
            gfx::ITexture *sheetTexture,
            component::Position stoodPosition,
            component::AnimationState posedState);

        bool beginLampCarry(voxel::VoxelPosition position);

        void carryLamp();

        void duplicateTile(
            geometry::GridCell fromCell, geometry::GridCell toCell);

        [[nodiscard]] bool mayAdjoin(
            tilemap::Tile oneTile, tilemap::Tile otherTile);

        [[nodiscard]] bool beginSliderDrag(
            const ui::Interactions &interactions);

        void undo();

        void redo();

        void growChunk();

        void settleHistory(
            std::optional<map::Snapshot> stepSnapshot,
            const std::string &doneLabel,
            const std::string &nothing);

        void pathTo(input::Position position);

        void standPlayerAt(std::int32_t x, std::int32_t z);

        void takeExit();

        [[nodiscard]] std::string progressPath() const;

        void savePlayerProgress();

        void setBindings(KeyBindings keyBindings);

        [[nodiscard]] input::KeyModifiers heldModifiers() const noexcept;

        [[nodiscard]] bool matchesChord(
            Action action, input::Key key) const;

        [[nodiscard]] bool matchesChordWithShift(
            Action action, input::Key key) const;

        void applyWalkKey(input::Key key, bool down);

        void applyRunKey(input::Key key, bool down);

        [[nodiscard]] bool handleBindingsKey(
            const input::KeyPressed &pressedKey);

        [[nodiscard]] std::string chordsPath() const;

        [[nodiscard]] bool layoutModals(ui::Context &context);

        [[nodiscard]] bool handleModalWidgets(
            const ui::Interactions &interactions);

        [[nodiscard]] bool consumeTextInput(
            const input::KeyPressed &pressedKey);
        [[nodiscard]] std::uint8_t glowOf(std::size_t ink) const;

        void carryInk();

        void recolorInk(gfx::Color nextColor);
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
        [[nodiscard]] bool handleWidgets(
            const ui::Interactions &interactions);

        void onKeyReleased(const input::KeyReleased &releasedEvent);
        void onKeyPressed(const input::KeyPressed &pressedKey);

        void frame(
            std::chrono::time_point<std::chrono::system_clock>
                startedAt);
        void drawWorldView(
            const ui::Frame &frame,
            std::chrono::time_point<std::chrono::system_clock>
                startedAt);

        void drawWorldOverlays(
            const ui::Frame &frame,
            const gfx::Camera3D &camera,
            const gfx::Mat4 &modelMatrix);
        void drawAtlasesView(
            const ui::Frame &frame,
            std::chrono::time_point<std::chrono::system_clock>
                startedAt);
        void drawCharacterView(
            const ui::Frame &frame,
            std::chrono::time_point<std::chrono::system_clock>
                startedAt);
        void drawIconsView(
            const ui::Frame &frame,
            std::chrono::time_point<std::chrono::system_clock>
                startedAt);

        void drawPlanView(
            const ui::Frame &frame,
            std::chrono::time_point<std::chrono::system_clock>
                startedAt);

        void setView(map::View nextView);
    };

}
