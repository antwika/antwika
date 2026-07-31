#include "antwika/atlas_editor/EditorState.hpp"

#include <cstddef>
#include <optional>
#include <utility>

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

    std::optional<Pixel> EditorState::hovered() const noexcept
    {
        return under;
    }

    const std::string &EditorState::status() const noexcept
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

    void EditorState::applyAt(const Point point) noexcept
    {
        moveTo(point);

        const Pixel pixel = pixelAt(where, point);

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

    void EditorState::eraseAt(const Point point) noexcept
    {
        moveTo(point);

        if (sheet.set(pixelAt(where, point), kClear))
        {
            ++changes;
        }
    }

    void EditorState::replace(Canvas image)
    {
        sheet = std::move(image);
        savedRevision = sheet.revision();
        where = centredView(area, sheet.size(), kOpeningZoom);
        under = std::nullopt;
        ++read;
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

    void EditorState::setStatus(std::string text)
    {
        message = std::move(text);
    }

} // namespace antwika::atlas_editor
