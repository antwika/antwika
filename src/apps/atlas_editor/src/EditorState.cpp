#include "antwika/atlas_editor/EditorState.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

#include "antwika/atlas_editor/AtlasForm.hpp"
#include "antwika/atlas_editor/AtlasMeta.hpp"
#include "antwika/atlas_editor/EditorTheme.hpp"
#include "antwika/atlas_editor/Palette.hpp"
#include "antwika/atlas_editor/Shapes.hpp"

namespace antwika::atlas_editor
{

    namespace
    {
        constexpr Color kClear{
            .red = 0, .green = 0, .blue = 0, .alpha = 0};

        constexpr std::size_t kOpeningZoom = 0;

        constexpr std::size_t kUndoDepth = 64;
    }

    EditorState::EditorState(
        Canvas image, const TileGrid tiles, const Size canvas)
        : sheet(std::move(image)),
          atlas(metaFor(tiles, sheet.size())),
          area(canvas),
          where(centredView(canvas, sheet.size(), kOpeningZoom)),
          paint(defaultPalette().front()),
          swatch(0)
    {
    }

    const Canvas &EditorState::image() const noexcept
    {
        return sheet;
    }

    CanvasView EditorState::view() const noexcept
    {
        return where;
    }

    Size EditorState::canvas() const noexcept
    {
        return area;
    }

    TileGrid EditorState::tiles() const noexcept
    {
        return tilesOf(atlas);
    }

    const AtlasMeta &EditorState::meta() const noexcept
    {
        return atlas;
    }

    const AtlasForm &EditorState::form() const noexcept
    {
        return asking;
    }

    std::size_t EditorState::formField() const noexcept
    {
        return askingField;
    }

    std::size_t EditorState::formCaret() const noexcept
    {
        return askingCaret;
    }

    Tool EditorState::tool() const noexcept
    {
        return selected;
    }

    Color EditorState::color() const noexcept
    {
        return paint;
    }

    std::optional<std::size_t> EditorState::colorIndex() const noexcept
    {
        return swatch;
    }

    bool EditorState::inkVisible() const noexcept
    {
        return inkShown;
    }

    std::optional<std::size_t> EditorState::inkDrag() const noexcept
    {
        return inkDragging;
    }

    void EditorState::toggleInk() noexcept
    {
        inkShown = !inkShown;
    }

    void EditorState::setInk(const Color ink) noexcept
    {
        paint = ink;
        swatch = std::nullopt;
    }

    void EditorState::setInkDrag(
        const std::optional<std::size_t> channel) noexcept
    {
        inkDragging = channel;
    }

    bool EditorState::pixelGridVisible() const noexcept
    {
        return showPixels;
    }

    Menu EditorState::openMenu() const noexcept
    {
        return shown;
    }

    Modal EditorState::openModal() const noexcept
    {
        return asked;
    }

    const std::vector<FileEntry> &EditorState::files() const noexcept
    {
        return listing;
    }

    const std::string &EditorState::directory() const noexcept
    {
        return browsing;
    }

    std::size_t EditorState::fileScroll() const noexcept
    {
        return listingScroll;
    }

    const std::string &EditorState::fileName() const noexcept
    {
        return named;
    }

    std::size_t EditorState::fileCaret() const noexcept
    {
        return namedCaret;
    }

    bool EditorState::gridVisible() const noexcept
    {
        return showGrid;
    }

    bool EditorState::guidesVisible() const noexcept
    {
        return showGuides;
    }

    bool EditorState::pivotVisible() const noexcept
    {
        return showPivot;
    }

    bool EditorState::pointerBorderVisible() const noexcept
    {
        return showPointerBorder;
    }

    std::optional<SpriteGuides> EditorState::guides() const noexcept
    {
        return guidesOf(atlas);
    }

    std::optional<Pixel> EditorState::hovered() const noexcept
    {
        return under;
    }

    const std::optional<StatusMessage> &EditorState::status()
        const noexcept
    {
        return message;
    }

    std::uint64_t EditorState::edits() const noexcept
    {
        return changes;
    }

    std::uint64_t EditorState::ticks() const noexcept
    {
        return stepped;
    }

    std::uint32_t EditorState::saves() const noexcept
    {
        return written;
    }

    std::uint32_t EditorState::loads() const noexcept
    {
        return read;
    }

    bool EditorState::unsaved() const noexcept
    {
        return sheet.revision() != savedRevision;
    }

    std::uint64_t EditorState::savedAtRevision() const noexcept
    {
        return savedRevision;
    }

    void EditorState::selectTool(const Tool tool) noexcept
    {
        selected = tool;
    }

    void EditorState::showMenu(const Menu menu) noexcept
    {
        shown = menu;
    }

    void EditorState::showModal(
        const Modal modal,
        std::string directory,
        std::vector<FileEntry> found)
    {
        asked = modal;
        shown = Menu::None;

        browse(std::move(directory), std::move(found));
    }

    void EditorState::browse(
        std::string directory, std::vector<FileEntry> found)
    {
        browsing = std::move(directory);
        listing = std::move(found);
        listingScroll = 0;
    }

    void EditorState::scrollFiles(const std::int64_t by) noexcept
    {
        const auto shown = filesShownIn(area, labelsAbove(asked));

        const auto last =
            listing.size() > shown ? listing.size() - shown : 0U;

        const auto moved =
            static_cast<std::int64_t>(listingScroll) + by;

        listingScroll = static_cast<std::size_t>(
            std::clamp<std::int64_t>(
                moved, 0, static_cast<std::int64_t>(last)));
    }

    void EditorState::closeModal() noexcept
    {
        asked = Modal::None;
        listing.clear();
    }

    void EditorState::showNewAtlas()
    {
        asked = Modal::New;
        shown = Menu::None;
        asking = formOf(atlas);
        askingField = 0;
        askingCaret = antwika::ui::kCaretAtEnd;
    }

    void EditorState::focusField(const std::size_t field) noexcept
    {
        if (field >= kAtlasFieldCount)
        {
            return;
        }

        askingField = field;
        askingCaret = antwika::ui::kCaretAtEnd;
    }

    void EditorState::setFormField(
        std::string text, const std::size_t caret)
    {
        asking.values[askingField] = std::move(text);
        askingCaret = caret;
    }

    void EditorState::takePreset(const std::size_t preset)
    {
        asking = presetForm(preset);
        askingCaret = antwika::ui::kCaretAtEnd;
    }

    void EditorState::turnKind() noexcept
    {
        asking.kind = asking.kind == AtlasKind::Isometric
                          ? AtlasKind::Flat
                          : AtlasKind::Isometric;
    }

    void EditorState::adoptMeta(const AtlasMeta &meta)
    {
        atlas = counted(meta, sheet.size());
    }

    void EditorState::openAtlas(const AtlasMeta &meta)
    {
        atlas = counted(meta, sheetSizeOf(meta));
        sheet = Canvas::blank(sheetSizeOf(atlas));
        savedRevision = sheet.revision();
        where = centredView(area, sheet.size(), kOpeningZoom);
        under = std::nullopt;

        forgetStrokes();
        clearSelection();
    }

    void EditorState::setFileName(
        std::string name, const std::size_t caret)
    {
        named = std::move(name);
        namedCaret = caret;
    }

    void EditorState::selectColor(const std::size_t index) noexcept
    {
        const auto palette = defaultPalette();

        if (index >= palette.size())
        {
            return;
        }

        paint = palette[index];
        swatch = index;
    }

    void EditorState::toggleGrid() noexcept
    {
        showGrid = !showGrid;
    }

    void EditorState::toggleGuides() noexcept
    {
        showGuides = !showGuides;
    }

    void EditorState::togglePivot() noexcept
    {
        showPivot = !showPivot;
    }

    void EditorState::togglePointerBorder() noexcept
    {
        showPointerBorder = !showPointerBorder;
    }

    void EditorState::togglePixelGrid() noexcept
    {
        showPixels = !showPixels;
    }

    void EditorState::zoomIn(const Point anchor) noexcept
    {
        where = zoomedIn(where, anchor);
    }

    void EditorState::zoomOut(const Point anchor) noexcept
    {
        where = zoomedOut(where, anchor);
    }

    void EditorState::panBy(const Point by) noexcept
    {
        where = pannedBy(where, by);
    }

    void EditorState::resetView() noexcept
    {
        where = centredView(area, sheet.size(), kOpeningZoom);
    }

    void EditorState::moveTo(const Point point) noexcept
    {
        under = pixelAt(where, point);
    }

    const PreviewPane &EditorState::preview() const noexcept
    {
        return pane;
    }

    void EditorState::togglePreview() noexcept
    {
        pane.open = !pane.open;
    }

    void EditorState::toggleAutoFocus() noexcept
    {
        pane.autoFocus = !pane.autoFocus;
    }

    void EditorState::setPreviewRatio(const std::uint32_t ratio) noexcept
    {
        pane.ratio = std::min(ratio, antwika::ui::kWholeSplit);
    }

    void EditorState::setPreviewDragging(const bool dragging) noexcept
    {
        pane.dragging = dragging;
    }

    void EditorState::zoomPreviewIn(const Point anchor) noexcept
    {
        pane.view = zoomedIn(pane.view, anchor);
        pane.autoFocus = false;
    }

    void EditorState::zoomPreviewOut(const Point anchor) noexcept
    {
        pane.view = zoomedOut(pane.view, anchor);
        pane.autoFocus = false;
    }

    void EditorState::panPreviewBy(const Point by) noexcept
    {
        pane.view = pannedBy(pane.view, by);
        pane.autoFocus = false;
    }

    void EditorState::focusPreviewOn(const PreviewPane &framed) noexcept
    {
        pane.view = framed.view;
    }

    void EditorState::noteTouched(const Pixel pixel) noexcept
    {
        pane.focused = slotAt(tiles(), sheet.size(), pixel);
    }

    void EditorState::fillFrom(const Pixel start)
    {
        if (!sheet.holds(start))
        {
            return;
        }

        const Color target = sheet.at(start);

        if (target == paint)
        {
            return;
        }

        std::vector<Pixel> pending{start};

        while (!pending.empty())
        {
            const Pixel pixel = pending.back();
            pending.pop_back();

            if (sheet.at(pixel) != target)
            {
                continue;
            }

            sheet.set(pixel, paint);
            ++changes;

            const std::array<Pixel, 4> around{
                Pixel{.x = pixel.x - 1, .y = pixel.y},
                Pixel{.x = pixel.x + 1, .y = pixel.y},
                Pixel{.x = pixel.x, .y = pixel.y - 1},
                Pixel{.x = pixel.x, .y = pixel.y + 1}};

            for (const Pixel next : around)
            {
                if (sheet.holds(next) && sheet.at(next) == target)
                {
                    pending.push_back(next);
                }
            }
        }
    }

    void EditorState::applyAt(const Point point)
    {
        moveTo(point);

        const Pixel pixel = pixelAt(where, point);

        if (selected == Tool::Select)
        {
            return;
        }

        if (selected == Tool::Fill)
        {
            fillFrom(pixel);
            return;
        }

        if (selected == Tool::Pick)
        {
            if (sheet.holds(pixel))
            {
                paint = sheet.at(pixel);
                swatch = std::nullopt;
            }

            return;
        }

        if (sheet.set(pixel, selected == Tool::Erase ? kClear : paint))
        {
            ++changes;
        }

        noteTouched(pixel);
    }

    void EditorState::eraseAt(const Point point)
    {
        moveTo(point);

        const Pixel pixel = pixelAt(where, point);

        if (sheet.set(pixel, kClear))
        {
            ++changes;
        }

        noteTouched(pixel);
    }

    std::optional<Selection> EditorState::selection() const noexcept
    {
        return marked;
    }

    std::optional<Selection> EditorState::shownSelection() const noexcept
    {
        if (!gesture.has_value() || selected != Tool::Select)
        {
            return marked;
        }

        if (!gesture->carrying)
        {
            return selectionBetween(gesture->from, gesture->to);
        }

        return movedBy(
            *marked,
            gesture->to.x - gesture->from.x,
            gesture->to.y - gesture->from.y);
    }

    bool EditorState::hasClipboard() const noexcept
    {
        return clipboard.has_value();
    }

    const std::optional<Canvas> &
    EditorState::clipboardImage() const noexcept
    {
        return clipboard;
    }

    std::optional<Gesture> EditorState::currentGesture() const noexcept
    {
        return gesture;
    }

    std::optional<Gesture> EditorState::shownStroke() const noexcept
    {
        return drawsShape(selected) ? gesture : std::nullopt;
    }

    Canvas EditorState::lift(const Selection &area) const
    {
        Canvas taken = Canvas::blank(area.size);

        for (std::uint32_t down = 0; down < area.size.height; ++down)
        {
            for (std::uint32_t across = 0; across < area.size.width;
                 ++across)
            {
                const Pixel from{
                    .x = area.origin.x + static_cast<std::int32_t>(across),
                    .y = area.origin.y + static_cast<std::int32_t>(down)};

                taken.set(
                    Pixel{
                        .x = static_cast<std::int32_t>(across),
                        .y = static_cast<std::int32_t>(down)},
                    sheet.at(from));
            }
        }

        return taken;
    }

    void EditorState::stamp(const Canvas &clip, const Pixel at)
    {
        const Size extent = clip.size();

        for (std::uint32_t down = 0; down < extent.height; ++down)
        {
            for (std::uint32_t across = 0; across < extent.width; ++across)
            {
                const Pixel read{
                    .x = static_cast<std::int32_t>(across),
                    .y = static_cast<std::int32_t>(down)};
                const Pixel write{
                    .x = at.x + static_cast<std::int32_t>(across),
                    .y = at.y + static_cast<std::int32_t>(down)};

                if (sheet.set(write, clip.at(read)))
                {
                    ++changes;
                }
            }
        }
    }

    void EditorState::clearRegion(const Selection &area)
    {
        for (std::uint32_t down = 0; down < area.size.height; ++down)
        {
            for (std::uint32_t across = 0; across < area.size.width;
                 ++across)
            {
                const Pixel pixel{
                    .x = area.origin.x + static_cast<std::int32_t>(across),
                    .y = area.origin.y + static_cast<std::int32_t>(down)};

                if (sheet.set(pixel, kClear))
                {
                    ++changes;
                }
            }
        }
    }

    void EditorState::beginSelecting(const Point point) noexcept
    {
        moveTo(point);

        const Pixel pixel = pixelAt(where, point);

        const bool carrying =
            marked.has_value() && contains(*marked, pixel);

        gesture = Gesture{
            .carrying = carrying, .from = pixel, .to = pixel};
    }

    void EditorState::dragSelectionTo(const Point point) noexcept
    {
        moveTo(point);

        if (!gesture.has_value())
        {
            return;
        }

        gesture->to = pixelAt(where, point);
    }

    void EditorState::finishSelecting(const Point point)
    {
        moveTo(point);

        if (!gesture.has_value())
        {
            return;
        }

        gesture->to = pixelAt(where, point);

        const Gesture drag = *gesture;
        gesture.reset();

        if (!drag.carrying)
        {
            marked = clampedTo(
                selectionBetween(drag.from, drag.to), sheet.size());
            return;
        }

        const Selection source = *marked;
        const Selection lands = movedBy(
            source,
            drag.to.x - drag.from.x,
            drag.to.y - drag.from.y);

        marked = clampedTo(lands, sheet.size());

        if (!marked.has_value())
        {
            return;
        }

        const Canvas carried = lift(source);

        clearRegion(source);

        stamp(carried, lands.origin);
    }

    void EditorState::beginStroke(const Point point) noexcept
    {
        moveTo(point);

        const Pixel pixel = pixelAt(where, point);

        gesture = Gesture{
            .carrying = false, .from = pixel, .to = pixel};
    }

    void EditorState::dragStrokeTo(const Point point) noexcept
    {
        dragSelectionTo(point);
    }

    void EditorState::finishStroke(const Point point)
    {
        moveTo(point);

        if (!gesture.has_value())
        {
            return;
        }

        const Gesture drawn = *gesture;
        gesture.reset();

        for (const Pixel pixel : shapePixels(
                 selected,
                 drawn.from,
                 pixelAt(where, point),
                 sheet.size()))
        {
            if (sheet.set(pixel, paint))
            {
                ++changes;
            }
        }
    }

    void EditorState::clearSelection() noexcept
    {
        marked.reset();
        gesture.reset();
    }

    void EditorState::copySelection()
    {
        if (!marked.has_value())
        {
            return;
        }

        clipboard = lift(*marked);
    }

    void EditorState::cutSelection()
    {
        if (!marked.has_value())
        {
            return;
        }

        clipboard = lift(*marked);
        clearRegion(*marked);
    }

    void EditorState::eraseSelection()
    {
        if (!marked.has_value())
        {
            return;
        }

        clearRegion(*marked);
    }

    void EditorState::pasteClipboard()
    {
        if (!clipboard.has_value() || !under.has_value())
        {
            return;
        }

        stamp(*clipboard, *under);

        marked = clampedTo(
            Selection{.origin = *under, .size = clipboard->size()},
            sheet.size());
    }

    void EditorState::replace(Canvas image)
    {
        sheet = std::move(image);
        atlas = counted(atlas, sheet.size());
        savedRevision = sheet.revision();
        where = centredView(area, sheet.size(), kOpeningZoom);
        under = std::nullopt;

        forgetStrokes();
        clearSelection();
        ++read;
    }

    void EditorState::forgetStrokes()
    {
        pending.reset();
        undone.clear();
        redone.clear();
    }

    void EditorState::openStroke()
    {
        pending = sheet.bitmap();
        pendingRevision = sheet.revision();
    }

    void EditorState::closeStroke()
    {
        if (!pending.has_value())
        {
            return;
        }

        if (sheet.revision() != pendingRevision)
        {
            undone.push_back(std::move(*pending));
            redone.clear();

            if (undone.size() > kUndoDepth)
            {
                undone.erase(undone.begin());
            }
        }

        pending.reset();
    }

    void EditorState::undo()
    {
        if (undone.empty())
        {
            return;
        }

        redone.push_back(sheet.bitmap());
        sheet = Canvas(std::move(undone.back()), sheet.revision() + 1);
        undone.pop_back();

        clearSelection();
    }

    void EditorState::redo()
    {
        if (redone.empty())
        {
            return;
        }

        undone.push_back(sheet.bitmap());
        sheet = Canvas(std::move(redone.back()), sheet.revision() + 1);
        redone.pop_back();

        clearSelection();
    }

    std::size_t EditorState::undoDepth() const noexcept
    {
        return undone.size();
    }

    std::size_t EditorState::redoDepth() const noexcept
    {
        return redone.size();
    }

    void EditorState::restore(SessionRestore snapshot)
    {
        sheet = std::move(snapshot.sheet);
        clipboard = std::move(snapshot.clipboard);
        where = snapshot.view;
        selected = snapshot.tool;
        paint = snapshot.paint;
        swatch = snapshot.swatch;
        showGrid = snapshot.showGrid;
        showGuides = snapshot.showGuides;
        showPivot = snapshot.showPivot;
        showPointerBorder = snapshot.showPointerBorder;
        showPixels = snapshot.showPixels;
        under = snapshot.under;
        gesture = snapshot.gesture;
        pane = snapshot.preview;

        marked = snapshot.marked.has_value()
                     ? clampedTo(*snapshot.marked, sheet.size())
                     : std::nullopt;

        message.reset();
        forgetStrokes();

        changes = snapshot.changes;
        stepped = snapshot.stepped;
        written = snapshot.written;
        read = snapshot.read;
        savedRevision = snapshot.savedRevision;
    }

    void EditorState::markSaved() noexcept
    {
        savedRevision = sheet.revision();
        ++written;
    }

    void EditorState::noteTick() noexcept
    {
        ++stepped;
    }

    void EditorState::setStatus(StatusMessage text)
    {
        message = std::move(text);
    }

}
