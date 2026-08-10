#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <antwika/enums/Enumeration.hpp>
#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/io/FileList.hpp>
#include <antwika/tilemap/Rgb.hpp>
#include <antwika/tilemap/TerrainClass.hpp>
#include <antwika/ui/Interactions.hpp>
#include <antwika/ui/Keyboard.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/map_editor/EditorState.hpp"
#include "antwika/map_editor/PaletteMath.hpp"

namespace antwika::map_editor
{

    inline constexpr std::int32_t kMapViewWidth = 320;

    inline constexpr std::int32_t kMenuBarHeight = 10;

    inline constexpr std::int32_t kMapViewHeight = 270 - kMenuBarHeight;

    inline constexpr std::size_t kZoomStepCount = 5;

    inline constexpr std::array<float, kZoomStepCount> kZoomSteps{
        0.5F, 1.0F, 2.0F, 3.0F, 4.0F};

    struct MapCamera final
    {
        std::size_t step = 1;
        float panX = 0.0F;
        float panY = 0.0F;

        [[nodiscard]] float zoom() const noexcept
        {
            return kZoomSteps[step % kZoomStepCount];
        }
    };

    /**
     * @brief Clamps the pan so part of the map stays in view.
     *
     * @param mapWidth The map width in unzoomed map pixels.
     * @param mapHeight The map height in unzoomed map pixels.
     *
     * Ensures: at least three tiles' worth of map remains inside
     *          the viewport on each axis.
     */
    void clampCamera(
        MapCamera &camera, float mapWidth, float mapHeight);

    /**
     * @brief Steps the zoom while keeping the anchor point still.
     *
     * @param anchorX The anchor in viewport-local canvas pixels.
     * @param anchorY The anchor in viewport-local canvas pixels.
     * @param direction Positive steps in and negative steps out.
     */
    void zoomAt(
        MapCamera &camera,
        float anchorX,
        float anchorY,
        std::int32_t direction,
        float mapWidth,
        float mapHeight);

    enum class GestureKind : std::uint8_t
    {
        Press = 0,
        Move,
        Release,
    };

    struct MapGesture final
    {
        GestureKind kind = GestureKind::Press;
        geometry::GridCell cell{};
        SignedCell signedCell{};
    };

    struct SheetGesture final
    {
        GestureKind kind = GestureKind::Press;
        gfx::Point pixel{};
        bool ink = true;
    };

    struct InputFold final
    {
        std::vector<input::InputEvent> events{};
        std::optional<gfx::Point> canvasPointer{};
        bool down = false;
        bool pressed = false;
        std::vector<ui::Key> uiKeys{};
        std::string typed{};
        std::vector<MapGesture> gestures{};
        std::vector<SheetGesture> sheetGestures{};
        bool panning = false;
        gfx::Point panAnchor{};
        bool quit = false;
        bool consoleVisible = false;
        std::int32_t consoleHeightCanvas = 0;
    };

    enum class EditorView : std::uint8_t
    {
        Map = 0,
        Tiles,
        Characters,
    };

    struct SheetDoc final
    {
        gfx::Bitmap image{};
        std::vector<gfx::Bitmap> undoStack{};
        std::vector<gfx::Bitmap> redoStack{};
        bool dirty = false;
        std::uint64_t revision = 0;
    };

    struct TileSheets final
    {
        std::filesystem::path directory{};
        std::array<SheetDoc, enums::kCount<tilemap::TerrainClass>>
            docs{};
        bool stroke = false;
        bool strokeInk = true;
    };

    struct FieldBuffer final
    {
        std::string text{};
        std::size_t cursor = 0;
    };

    struct UiSession final
    {
        ui::WidgetId focus = ui::kNoWidget;
        std::size_t placeKind = 0;
        bool placeOpen = false;
        std::optional<std::size_t> openMenu{};
        std::optional<std::size_t> selected{};
        FieldBuffer idField{};
        FieldBuffer targetMapField{};
        FieldBuffer targetEntryField{};
        FieldBuffer tagsField{};
        ui::Interactions acted{};
        bool pointerOverUi = false;
        bool enemyOpen = false;
    };

    enum class DialogMode : std::uint8_t
    {
        None = 0,
        Open,
        SaveAs,
    };

    inline constexpr std::size_t kDialogRows = 10;

    struct FileDialog final
    {
        DialogMode mode = DialogMode::None;
        std::string directory{};
        std::vector<io::FileEntry> entries{};
        std::size_t page = 0;
        FieldBuffer nameField{};
        std::string message{};

        [[nodiscard]] bool open() const noexcept
        {
            return mode != DialogMode::None;
        }
    };

    struct PaletteDialog final
    {
        bool open = false;
        bool paperActive = false;
        Hsv hsv{};
        FieldBuffer hexField{};
        tilemap::Rgb savedInk{};
        tilemap::Rgb savedPaper{};
        bool svDragging = false;
        bool hueDragging = false;
    };

    struct CharacterDoc final
    {
        std::string name{};
        SheetDoc sheet{};
    };

    struct CharacterSet final
    {
        std::filesystem::path directory{};
        std::vector<CharacterDoc> list{};
        std::size_t selected = 0;
        FieldBuffer nameField{};
        bool confirmDelete = false;
        std::string message{};
    };

    struct EditorStore final
    {
        EditorState state;
        InputFold input{};
        UiSession ui{};
        MapCamera camera{};
        FileDialog dialog{};
        PaletteDialog palette{};
        EditorView view = EditorView::Map;
        TileSheets tiles{};
        CharacterSet characters{};
        std::uint32_t uiScale = 3;
        std::optional<std::uint32_t> pendingUiScale{};
    };

    /**
     * @brief Whether a modal dialog owns the pointer and keys.
     *
     * Ensures: true while the file dialog or the palette dialog is
     *          open, during which map and workspace input is
     *          suppressed.
     */
    [[nodiscard]] inline bool modalOpen(
        const EditorStore &store) noexcept
    {
        return store.dialog.open() || store.palette.open;
    }

    /**
     * @brief The sheet document the active workspace edits.
     *
     * @return The selected terrain's sheet in the tiles view, the
     *         selected character's sheet in the characters view,
     *         and null in the map view or with nothing to edit.
     */
    [[nodiscard]] SheetDoc *activeSheet(EditorStore &store);

    /**
     * @brief Steps the view through Map, Tiles, and Characters.
     */
    void cycleEditorView(EditorStore &store);

    /**
     * @brief Opens the file dialog in the map's directory.
     *
     * Ensures: the listing shows directories and .json files only,
     *          and Save As pre-fills the current file name.
     */
    void openFileDialog(EditorStore &store, DialogMode mode);

    /**
     * @brief Re-lists the dialog's directory.
     *
     * Ensures: only directories and .json files remain, and the
     *          page index returns to the first page.
     */
    void refreshDialogEntries(FileDialog &dialog);

    /**
     * @brief Reloads the entity field buffers from the selection.
     *
     * Ensures: the buffers are cleared when nothing is selected or
     *          the selection index is out of range.
     */
    void loadEntityBuffers(EditorStore &store);

    [[nodiscard]] std::string joinTags(
        const std::vector<std::string> &tags);

    [[nodiscard]] std::vector<std::string> splitTags(
        const std::string &joined);

}
