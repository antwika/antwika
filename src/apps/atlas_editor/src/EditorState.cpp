#include "antwika/atlas_editor/EditorState.hpp"

#include <array>
#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

#include "antwika/atlas_editor/Palette.hpp"

namespace antwika::atlas_editor
{

    namespace
    {
        constexpr Color kClear{
            .red = 0, .green = 0, .blue = 0, .alpha = 0};

        // Widest first, so a sheet opens showing all of itself.
        constexpr std::size_t kOpeningZoom = 0;
    } // namespace

    EditorState::EditorState(
        Canvas image, const TileGrid tiles, const Size canvas)
        : sheet(std::move(image)),
          grid(tiles),
          area(canvas),
          where(centredView(canvas, sheet.size(), kOpeningZoom)),
          // Worked out once, here.
          // Nothing later changes the grid these come out of.
          outlines(guidesForTile(tiles)),
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
        return grid;
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

    bool EditorState::gridVisible() const noexcept
    {
        return showGrid;
    }

    bool EditorState::guidesVisible() const noexcept
    {
        return showGuides;
    }

    std::optional<SpriteGuides> EditorState::guides() const noexcept
    {
        return outlines;
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

    // Four-connected, bounded by the sheet.
    // Walked from an explicit stack of the pixels still to look at.
    // Deterministic without needing to be ordered.
    // Every pixel of the region ends the same colour whatever the order.
    // So there is no tie here for a total order to break.
    void EditorState::fillFrom(const Pixel start)
    {
        if (!sheet.holds(start))
        {
            return;
        }

        const Color target = sheet.at(start);

        // A fill with the colour already there spreads nothing.
        // Without this the walk below would never terminate.
        // Every pixel it wrote would still hold the colour it looks for.
        if (target == paint)
        {
            return;
        }

        std::vector<Pixel> pending{start};

        while (!pending.empty())
        {
            const Pixel pixel = pending.back();
            pending.pop_back();

            // Only pixels inside the sheet are ever pushed below.
            // So the sheet's edge is not tested again here.
            // What is tested is that this one is still unreached.
            // Two neighbours can push one pixel before either is taken.
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

        // Select's left button is a gesture rather than a brush.
        // Which is beginSelecting() and the two that follow it.
        // So there is no pixel for a stroke to put down here.
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
            // Beyond the sheet's edge there is transparent nothing.
            // Taking that as a colour is a brush that paints nothing.
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
    }

    void EditorState::eraseAt(const Point point)
    {
        moveTo(point);

        if (sheet.set(pixelAt(where, point), kClear))
        {
            ++changes;
        }
    }

    std::optional<Selection> EditorState::selection() const noexcept
    {
        return marked;
    }

    std::optional<Selection> EditorState::shownSelection() const noexcept
    {
        if (!gesture.has_value())
        {
            return marked;
        }

        // A drawn rectangle is the two corners of the drag.
        // A carried one is the marked rectangle, slid by how far it came.
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

                // A pixel off the sheet is dropped by the canvas itself.
                // So a paste half off an edge lands the half that fits.
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

        // Inside the marked rectangle carries it.
        // Anywhere else draws a new one.
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

        // Where the button came up.
        // Which need not be where the last movement left it.
        // A release carries a position of its own.
        // And a drag can end with no further movement reported.
        gesture->to = pixelAt(where, point);

        const Gesture drag = *gesture;
        gesture.reset();

        // Only the part of it the sheet holds is ever marked.
        // A drag that left the sheet entirely marks nothing at all.
        if (!drag.carrying)
        {
            marked = clampedTo(
                selectionBetween(drag.from, drag.to), sheet.size());
            return;
        }

        // Carrying is only ever begun with a rectangle marked.
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

        // Copied out before the source is cleared.
        // So one carried a little way onto itself keeps its pixels.
        const Canvas carried = lift(source);

        clearRegion(source);

        // From the unclamped corner.
        // So a rectangle half off the sheet lands its half in place.
        stamp(carried, lands.origin);
    }

    void EditorState::clearSelection() noexcept
    {
        marked.reset();
        gesture.reset();
    }

    // Neither clamps, and neither has to.
    // A marked rectangle is only ever set through clampedTo().
    // So it is inside the sheet before anything here can act on it.
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

    void EditorState::pasteClipboard()
    {
        if (!clipboard.has_value() || !under.has_value())
        {
            return;
        }

        stamp(*clipboard, *under);

        // What landed becomes the marked rectangle.
        // So a paste can be carried on without being marked out again.
        marked = clampedTo(
            Selection{.origin = *under, .size = clipboard->size()},
            sheet.size());
    }

    void EditorState::replace(Canvas image)
    {
        sheet = std::move(image);
        savedRevision = sheet.revision();
        where = centredView(area, sheet.size(), kOpeningZoom);
        under = std::nullopt;

        // A rectangle on the old sheet is not one on the new sheet.
        // Whose size need not even be the same.
        // The clipboard survives, being nobody's sheet in particular.
        clearSelection();
        ++read;
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
        under = snapshot.under;
        gesture = snapshot.gesture;

        // Clamped again at the door, exactly as a finished drag is.
        // A rectangle a dump names has to be inside the sheet it names.
        // Or copy, cut and carry would index pixels that are not there.
        marked = snapshot.marked.has_value()
                     ? clampedTo(*snapshot.marked, sheet.size())
                     : std::nullopt;

        // Transient, and deliberately not part of a dump.
        // The last thing said is not part of what the session is.
        message.reset();

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

} // namespace antwika::atlas_editor
