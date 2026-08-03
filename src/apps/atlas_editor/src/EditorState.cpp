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

    void EditorState::setStatus(StatusMessage text)
    {
        message = std::move(text);
    }

} // namespace antwika::atlas_editor
