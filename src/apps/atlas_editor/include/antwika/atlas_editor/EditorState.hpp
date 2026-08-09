#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include <string>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>

#include <antwika/ui/TextFieldSpec.hpp>

#include "antwika/atlas_editor/AtlasForm.hpp"
#include "antwika/atlas_editor/AtlasMeta.hpp"
#include "antwika/atlas_editor/Canvas.hpp"
#include "antwika/atlas_editor/CanvasView.hpp"
#include "antwika/atlas_editor/FileList.hpp"
#include "antwika/atlas_editor/Ink.hpp"
#include "antwika/atlas_editor/Modal.hpp"
#include "antwika/atlas_editor/Pixel.hpp"
#include "antwika/atlas_editor/Preview.hpp"
#include "antwika/atlas_editor/Selection.hpp"
#include "antwika/atlas_editor/SpriteGuides.hpp"
#include "antwika/atlas_editor/StatusMessage.hpp"
#include "antwika/atlas_editor/TileGrid.hpp"
#include "antwika/atlas_editor/Tool.hpp"

namespace antwika::atlas_editor
{

    using antwika::gfx::Bitmap;
    using antwika::gfx::Color;
    using antwika::gfx::Point;
    using antwika::gfx::Size;

    enum class Menu : std::uint8_t
    {
        None = 0,

        File,

        View,

        Preset,
    };

    struct Gesture final
    {
        bool carrying = false;

        Pixel from{};

        Pixel to{};

        [[nodiscard]] bool operator==(const Gesture &other) const =
            default;
    };

    struct SessionRestore final
    {
        Canvas sheet;

        std::optional<Canvas> clipboard;

        CanvasView view;

        Tool tool = Tool::Paint;

        Color paint;

        std::optional<std::size_t> swatch;

        bool showGrid = true;

        bool showGuides = true;

        bool showPivot = false;

        bool showPointerBorder = true;

        bool showPixels = false;

        std::optional<Pixel> under;

        std::optional<Selection> marked;

        std::optional<Gesture> gesture;

        PreviewPane preview{};

        std::uint64_t changes = 0;

        std::uint64_t stepped = 0;

        std::uint32_t written = 0;

        std::uint32_t read = 0;

        std::uint64_t savedRevision = 0;
    };

    class EditorState final
    {
    public:
        EditorState(Canvas image, TileGrid tiles, Size canvas);

        [[nodiscard]] const Canvas &image() const noexcept;

        [[nodiscard]] CanvasView view() const noexcept;

        [[nodiscard]] Size canvas() const noexcept;

        [[nodiscard]] TileGrid tiles() const noexcept;

        [[nodiscard]] const AtlasMeta &meta() const noexcept;

        [[nodiscard]] const AtlasForm &form() const noexcept;

        [[nodiscard]] std::size_t formField() const noexcept;

        [[nodiscard]] std::size_t formCaret() const noexcept;

        [[nodiscard]] Tool tool() const noexcept;

        [[nodiscard]] Color color() const noexcept;

        [[nodiscard]] std::optional<std::size_t>
        colorIndex() const noexcept;

        [[nodiscard]] bool inkVisible() const noexcept;

        [[nodiscard]] std::optional<std::size_t>
        inkDrag() const noexcept;

        [[nodiscard]] Menu openMenu() const noexcept;

        [[nodiscard]] Modal openModal() const noexcept;

        [[nodiscard]] const std::vector<FileEntry> &
        files() const noexcept;

        [[nodiscard]] const std::string &directory() const noexcept;

        [[nodiscard]] std::size_t fileScroll() const noexcept;

        [[nodiscard]] const std::string &fileName() const noexcept;

        [[nodiscard]] std::size_t fileCaret() const noexcept;

        [[nodiscard]] bool gridVisible() const noexcept;

        [[nodiscard]] bool guidesVisible() const noexcept;

        [[nodiscard]] bool pivotVisible() const noexcept;

        [[nodiscard]] bool pointerBorderVisible() const noexcept;

        [[nodiscard]] bool pixelGridVisible() const noexcept;

        [[nodiscard]] std::optional<SpriteGuides> guides() const noexcept;

        [[nodiscard]] std::optional<Pixel> hovered() const noexcept;

        [[nodiscard]] std::optional<Selection> selection() const noexcept;

        [[nodiscard]] std::optional<Selection>
        shownSelection() const noexcept;

        [[nodiscard]] bool hasClipboard() const noexcept;

        [[nodiscard]] const std::optional<Canvas> &
        clipboardImage() const noexcept;

        [[nodiscard]] std::optional<Gesture>
        currentGesture() const noexcept;

        [[nodiscard]] std::optional<Gesture> shownStroke() const noexcept;

        [[nodiscard]] const PreviewPane &preview() const noexcept;

        [[nodiscard]] const std::optional<StatusMessage> &status()
            const noexcept;

        [[nodiscard]] std::uint64_t edits() const noexcept;

        [[nodiscard]] std::uint64_t ticks() const noexcept;

        [[nodiscard]] std::uint32_t saves() const noexcept;

        [[nodiscard]] std::uint32_t loads() const noexcept;

        [[nodiscard]] bool unsaved() const noexcept;

        [[nodiscard]] std::uint64_t savedAtRevision() const noexcept;

        void selectTool(Tool tool) noexcept;

        void showMenu(Menu menu) noexcept;

        void showModal(
            Modal modal,
            std::string directory,
            std::vector<FileEntry> listing);

        void browse(
            std::string directory, std::vector<FileEntry> listing);

        /**
         * @brief Slides the window of entries the explorer shows.
         *
         * @param by Entries to slide by; negative walks towards the
         *           start of the listing.
         *
         * Ensures: the window never starts past the last page of
         *          entries, nor before the first.
         */
        void scrollFiles(std::int64_t by) noexcept;

        void closeModal() noexcept;

        void showNewAtlas();

        void focusField(std::size_t field) noexcept;

        void setFormField(std::string text, std::size_t caret);

        void takePreset(std::size_t preset);

        void turnKind() noexcept;

        /**
         * @brief Lays a fresh sheet out to an atlas description.
         *
         * @param meta The atlas to lay out.
         *
         * Ensures: the sheet spans the columns and rows it names, and
         *          the grid and guides follow it.
         */
        void openAtlas(const AtlasMeta &meta);

        /**
         * @brief Takes on the atlas a loaded sheet came with.
         *
         * @param meta The atlas the sidecar described.
         *
         * Ensures: the columns and rows count the sheet in hand rather
         *          than the one the sidecar was written beside.
         */
        void adoptMeta(const AtlasMeta &meta);

        void setFileName(std::string name, std::size_t caret);

        void selectColor(std::size_t index) noexcept;

        void toggleInk() noexcept;

        void setInk(Color ink) noexcept;

        void setInkDrag(std::optional<std::size_t> channel) noexcept;

        void toggleGrid() noexcept;

        void toggleGuides() noexcept;

        void togglePivot() noexcept;

        void togglePointerBorder() noexcept;

        void togglePixelGrid() noexcept;

        void zoomIn(Point anchor) noexcept;

        void zoomOut(Point anchor) noexcept;

        void panBy(Point by) noexcept;

        void resetView() noexcept;

        void togglePreview() noexcept;

        void toggleAutoFocus() noexcept;

        void setPreviewRatio(std::uint32_t ratio) noexcept;

        void setPreviewDragging(bool dragging) noexcept;

        /**
         * @brief Zooms the preview about a point, by hand.
         *
         * @param anchor The canvas point to keep still.
         *
         * Ensures: auto-focus is off, because the reader has taken the
         *          pane somewhere of their own.
         */
        void zoomPreviewIn(Point anchor) noexcept;

        /**
         * @brief Zooms the preview out about a point, by hand.
         *
         * @param anchor The canvas point to keep still.
         *
         * Ensures: auto-focus is off, because the reader has taken the
         *          pane somewhere of their own.
         */
        void zoomPreviewOut(Point anchor) noexcept;

        /**
         * @brief Slides the preview, by hand.
         *
         * @param by How far to slide it in canvas pixels.
         *
         * Ensures: auto-focus is off, because the reader has taken the
         *          pane somewhere of their own.
         */
        void panPreviewBy(Point by) noexcept;

        void focusPreviewOn(const PreviewPane &framed) noexcept;

        void moveTo(Point point) noexcept;

        void applyAt(Point point);

        void eraseAt(Point point);

        void beginSelecting(Point point) noexcept;

        void dragSelectionTo(Point point) noexcept;

        void finishSelecting(Point point);

        void beginStroke(Point point) noexcept;

        void dragStrokeTo(Point point) noexcept;

        void finishStroke(Point point);

        void clearSelection() noexcept;

        void copySelection();

        void cutSelection();

        /**
         * @brief Empties the pixels the selection covers.
         *
         * Ensures: nothing is copied, so the clipboard is left as it
         *          was.
         */
        void eraseSelection();

        void pasteClipboard();

        void replace(Canvas image);

        void openStroke();

        void closeStroke();

        void undo();

        void redo();

        [[nodiscard]] std::size_t undoDepth() const noexcept;

        [[nodiscard]] std::size_t redoDepth() const noexcept;

        void restore(SessionRestore snapshot);

        void markSaved() noexcept;

        void noteTick() noexcept;

        void setStatus(StatusMessage message);

        /**
         * @brief Remembers which slot an edit landed in.
         *
         * @param pixel The sheet pixel the edit touched.
         *
         * Ensures: the pane follows the edit only while auto-focus is
         *          on.
         */
        void noteTouched(Pixel pixel) noexcept;

    private:
        void forgetStrokes();

        void fillFrom(Pixel start);

        [[nodiscard]] Canvas lift(const Selection &area) const;

        void stamp(const Canvas &clip, Pixel at);

        void clearRegion(const Selection &area);

        Canvas sheet;
        AtlasMeta atlas;
        Size area;
        CanvasView where;
        Tool selected = Tool::Paint;
        Menu shown = Menu::None;
        Modal asked = Modal::None;
        std::string browsing{"."};
        std::vector<FileEntry> listing;
        std::size_t listingScroll = 0;
        std::string named;
        std::size_t namedCaret = antwika::ui::kCaretAtEnd;
        AtlasForm asking;
        std::size_t askingField = 0;
        std::size_t askingCaret = antwika::ui::kCaretAtEnd;
        Color paint;
        std::optional<std::size_t> swatch;
        bool inkShown = false;
        std::optional<std::size_t> inkDragging{};
        bool showGrid = true;
        bool showGuides = true;
        bool showPivot = false;
        bool showPointerBorder = true;
        bool showPixels = false;
        std::optional<Pixel> under;
        std::optional<Selection> marked;
        std::optional<Gesture> gesture;
        PreviewPane pane{};
        std::optional<Canvas> clipboard;
        std::optional<StatusMessage> message;

        std::optional<Bitmap> pending{};
        std::uint64_t pendingRevision = 0;
        std::vector<Bitmap> undone;
        std::vector<Bitmap> redone;

        std::uint64_t changes = 0;
        std::uint64_t stepped = 0;
        std::uint32_t written = 0;
        std::uint32_t read = 0;
        std::uint64_t savedRevision = 0;
    };

}
