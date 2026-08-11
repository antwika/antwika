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
#include <antwika/gfx/Size.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/io/FileList.hpp>
#include <antwika/tilemap/Rgb.hpp>
#include <antwika/tilemap/TerrainClass.hpp>
#include <antwika/tileset/PixelClass.hpp>
#include <antwika/tileset/Tileset.hpp>
#include <antwika/ui/Interactions.hpp>
#include <antwika/ui/Keyboard.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/map_editor/EditorState.hpp"
#include "antwika/map_editor/Hotkeys.hpp"
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
        bool erase = false;
    };

    struct SheetGesture final
    {
        GestureKind kind = GestureKind::Press;
        gfx::Point pixel{};
        bool ink = true;
        bool ctrl = false;
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
        bool erasing = false;
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

    struct FieldBuffer final
    {
        std::string text{};
        std::size_t cursor = 0;
    };

    enum class TilesetTool : std::uint8_t
    {
        Draw = 0,
        Sockets,
        Decor,
        Select,
    };

    enum class MapTool : std::uint8_t
    {
        Paint = 0,
        Select,
    };

    enum class CharacterTool : std::uint8_t
    {
        Draw = 0,
        Select,
    };

    struct TilesetSelection final
    {
        std::size_t layer = 0;
        std::size_t sprite = 0;
        std::size_t frame = 0;

        [[nodiscard]] bool operator==(
            const TilesetSelection &other) const = default;
    };

    struct CellSpan final
    {
        geometry::GridCell origin{};
        std::uint32_t columns = 1;
        std::uint32_t rows = 1;
    };

    struct MapSelection final
    {
        std::optional<CellSpan> rect{};
        bool dragging = false;
        bool dragged = false;
        geometry::GridCell anchor{};
        geometry::GridCell focus{};
        bool moving = false;
        geometry::GridCell moveAnchor{};
        geometry::GridCell movePointer{};
    };

    struct PixelSpan final
    {
        gfx::Point origin{};
        std::int32_t width = 1;
        std::int32_t height = 1;
    };

    struct PixelSelection final
    {
        std::optional<PixelSpan> rect{};
        bool dragging = false;
        bool dragged = false;
        gfx::Point anchor{};
        gfx::Point focus{};
        bool moving = false;
        gfx::Point moveAnchor{};
        gfx::Point movePointer{};
    };

    struct TilesSelection final
    {
        PixelSelection pixels{};

        /**
         * @brief The open-tileset index the rect was made on.
         *
         * Ensures: a rect whose doc or ctx no longer matches the
         *          active document reads as no selection.
         */
        std::size_t doc = 0;
        TilesetSelection ctx{};
    };

    struct CharacterSelection final
    {
        PixelSelection pixels{};

        /**
         * @brief The character index the rect was made on.
         *
         * Ensures: a rect whose character no longer matches the
         *          selected one reads as no selection.
         */
        std::size_t character = 0;
    };

    struct PixelClipboard final
    {
        std::int32_t width = 0;
        std::int32_t height = 0;
        std::vector<tileset::PixelClass> pixels{};
    };

    struct TilesetSnapshot final
    {
        tileset::Tileset data{};
        TilesetSelection sel{};
    };

    struct TilesetDoc final
    {
        tileset::Tileset data{};
        std::filesystem::path path{};
        TilesetSelection sel{};
        std::vector<TilesetSnapshot> undoStack{};
        std::vector<TilesetSnapshot> redoStack{};
        bool dirty = false;
        std::uint64_t revision = 0;
    };

    struct TilesetWorkspace final
    {
        std::filesystem::path directory{};
        std::vector<TilesetDoc> open{};
        std::size_t active = 0;
        TilesetTool tool = TilesetTool::Draw;
        std::optional<std::size_t> activeSocket{};
        bool pickerOpen = false;
        std::size_t libraryPage = 0;
        bool stroke = false;
        bool strokeInk = true;
        bool decorStroke = false;
        bool drawPaper = false;
        bool confirmDeleteSprite = false;
        std::uint32_t previewSeed = 0;
        bool previewAuto = false;
        FieldBuffer socketNameField{};
        std::string message{};
    };

    struct NewTilesetDialog final
    {
        bool open = false;
        FieldBuffer nameField{};
        std::size_t terrain = 0;
        bool terrainOpen = false;
        std::string message{};
    };

    struct BindingsDialog final
    {
        bool open = false;
        std::array<
            std::size_t,
            enums::kCount<tilemap::TerrainClass>>
            chosen{};
        std::array<bool, enums::kCount<tilemap::TerrainClass>>
            pickerOpen{};
        std::string message{};
    };

    struct PickedSprite final
    {
        tilemap::TerrainClass terrain =
            tilemap::TerrainClass::Floor;
        std::uint16_t atlasRow = 0;
        std::string label{};
        std::uint64_t tick = 0;
    };

    struct PickerState final
    {
        bool active = false;
        std::optional<gfx::Point> pending{};
        std::optional<gfx::Point> walkCell{};
        std::size_t walkDepth = 0;
        std::optional<PickedSprite> picked{};
        std::string hover{};
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

    enum class DialogTarget : std::uint8_t
    {
        Map = 0,
        Tileset,
    };

    inline constexpr std::size_t kDialogRows = 10;

    struct FileDialog final
    {
        DialogMode mode = DialogMode::None;
        DialogTarget target = DialogTarget::Map;
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

    struct RulesDialog final
    {
        bool open = false;
        GenerationRules edit{};
        std::string message{};
    };

    struct KeysDialog final
    {
        bool open = false;
        std::optional<HotkeyAction> capturing{};
        std::string message{};
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
        CharacterTool tool = CharacterTool::Draw;
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
        RulesDialog rules{};
        NewTilesetDialog newTileset{};
        BindingsDialog bindings{};
        KeysDialog keys{};
        HotkeyBindings hotkeys = defaultHotkeyBindings();
        EditorView view = EditorView::Map;
        PickerState picker{};
        TilesetWorkspace tilesets{};
        CharacterSet characters{};
        MapTool mapTool = MapTool::Paint;
        MapSelection mapSelection{};
        TilesSelection tilesSelection{};
        CharacterSelection charSelection{};
        std::optional<Stamp> mapClipboard{};
        std::optional<PixelClipboard> pixelClipboard{};
        std::uint32_t uiScale = 3;
        std::optional<std::uint32_t> pendingUiScale{};
        bool fullscreen = false;
        bool pendingFullscreenToggle = false;
        bool pendingConfigWrite = false;
        gfx::Size windowSize{};
    };

    /**
     * @brief Whether a modal dialog owns the pointer and keys.
     *
     * Ensures: true while the file, palette, rules, new-tileset,
     *          bindings, or keys dialog is open, during which map
     *          and workspace input is suppressed.
     */
    [[nodiscard]] inline bool modalOpen(
        const EditorStore &store) noexcept
    {
        return store.dialog.open() || store.palette.open
               || store.rules.open || store.newTileset.open
               || store.bindings.open || store.keys.open;
    }

    /**
     * @brief The sheet document the active workspace edits.
     *
     * @return The selected character's sheet in the characters view,
     *         and null in the other views or with nothing to edit.
     */
    [[nodiscard]] SheetDoc *activeSheet(EditorStore &store);

    /**
     * @brief The tileset document the tiles view edits.
     *
     * @return The active open tileset, or null when none is open.
     */
    [[nodiscard]] TilesetDoc *activeTilesetDoc(EditorStore &store);

    [[nodiscard]] const TilesetDoc *activeTilesetDoc(
        const EditorStore &store);

    /**
     * @brief Finds the map pixel a canvas position lands on.
     *
     * @param canvas The pointer position in canvas pixels.
     * @return The position in unzoomed map pixels, matching the
     *         draw plan's screen coordinates.
     */
    [[nodiscard]] gfx::Point mapPointOf(
        gfx::Point canvas, const MapCamera &camera) noexcept;

    /**
     * @brief Toggles the sprite picker mode.
     *
     * Ensures: the pending pick, the stack walk, and the hover text
     *          reset, so a fresh mode starts at the stack bottom.
     */
    void togglePicker(EditorStore &store);

    /**
     * @brief Steps the view through Map, Tiles, and Characters.
     */
    void cycleEditorView(EditorStore &store);

    /**
     * @brief Steps the view through Characters, Tiles, and Map.
     */
    void cycleEditorViewBack(EditorStore &store);

    /**
     * @brief Opens the file dialog for maps or tilesets.
     *
     * Ensures: a map dialog lists directories and .json files in the
     *          map's directory, a tileset dialog lists the tileset
     *          directories under the tilesets directory, and Save As
     *          pre-fills the current name.
     */
    void openFileDialog(
        EditorStore &store,
        DialogMode mode,
        DialogTarget target = DialogTarget::Map);

    /**
     * @brief Re-lists the dialog's directory.
     *
     * Ensures: a map dialog keeps directories and .json files only, a
     *          tileset dialog lists tileset directories by name, and
     *          the page index returns to the first page.
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
