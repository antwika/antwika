#include "antwika/map_editor/Selection.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/tileset/Sprite.hpp>

#include "antwika/map_editor/CharacterSheets.hpp"
#include "antwika/map_editor/Commands.hpp"
#include "antwika/map_editor/SheetWorkspace.hpp"
#include "antwika/map_editor/TilesetWorkspace.hpp"

namespace antwika::map_editor
{

    namespace
    {
        using antwika::geometry::GridCell;
        using antwika::gfx::Color;
        using antwika::gfx::Point;
        using antwika::gfx::RectF;
        using antwika::tileset::kSpriteSide;
        using antwika::tileset::PixelClass;

        constexpr Color kMarqueeColor{
            .red = 255, .green = 255, .blue = 255};

        constexpr float kDashOn = 3.0F;

        constexpr float kDashPeriod = 6.0F;

        [[nodiscard]] std::optional<PixelSpan> clippedPixelSpan(
            const Point origin,
            const std::int32_t width,
            const std::int32_t height,
            const std::int32_t side)
        {
            const auto left = std::max(origin.x, 0);
            const auto top = std::max(origin.y, 0);
            const auto right = std::min(origin.x + width, side);
            const auto bottom = std::min(origin.y + height, side);

            if (left >= right || top >= bottom)
            {
                return std::nullopt;
            }

            return PixelSpan{
                .origin = {.x = left, .y = top},
                .width = right - left,
                .height = bottom - top};
        }

        [[nodiscard]] Point clampedEditorPixel(const Point canvas)
        {
            const auto side =
                static_cast<std::int32_t>(kSpriteSide);

            return Point{
                .x = std::clamp(
                    (canvas.x - kTilesetEditorLeft)
                        / kTilesetEditorZoom,
                    0,
                    side - 1),
                .y = std::clamp(
                    (canvas.y - kTilesetEditorTop)
                        / kTilesetEditorZoom,
                    0,
                    side - 1)};
        }

        [[nodiscard]] tileset::Sprite *activeSpriteOf(
            TilesetDoc &doc)
        {
            if (doc.sel.layer >= doc.data.layers.size())
            {
                return nullptr;
            }

            auto &sprites =
                doc.data.layers[doc.sel.layer].sprites;

            if (doc.sel.sprite >= sprites.size())
            {
                return nullptr;
            }

            return &sprites[doc.sel.sprite];
        }

        void ensureFrame(
            const TilesetSelection &sel, tileset::Sprite &sprite)
        {
            if (sel.frame < sprite.frameCount)
            {
                return;
            }

            for (auto frame = sprite.frameCount;
                 frame <= sel.frame;
                 ++frame)
            {
                sprite.frames[frame] = sprite.frames[0];
            }

            sprite.frameCount =
                static_cast<std::uint8_t>(sel.frame + 1);
        }

        [[nodiscard]] std::size_t frameOffsetOf(const Point pixel)
        {
            return static_cast<std::size_t>(
                pixel.y * static_cast<std::int32_t>(kSpriteSide)
                + pixel.x);
        }

        void finishTilesEdit(EditorStore &store, TilesetDoc &doc)
        {
            if (doc.undoStack.back().data == doc.data)
            {
                doc.undoStack.pop_back();
                return;
            }

            doc.dirty = true;
            ++doc.revision;
            store.tilesets.message.clear();
        }

        void finishSheetEdit(SheetDoc &doc)
        {
            if (doc.undoStack.back() == doc.image)
            {
                doc.undoStack.pop_back();
                return;
            }

            doc.dirty = true;
            ++doc.revision;
        }

        void copyMapSelection(EditorStore &store)
        {
            const auto span = mapSelectionSpan(store);

            if (!span.has_value())
            {
                return;
            }

            copyMapSpan(store, *span);
        }

        void cutMapSelection(EditorStore &store)
        {
            const auto span = mapSelectionSpan(store);

            if (!span.has_value())
            {
                return;
            }

            copyMapSpan(store, *span);
            clearMapSpan(store.state, *span);
        }

        void finishMapMove(EditorStore &store)
        {
            auto &sel = store.mapSelection;
            const auto span = mapSelectionSpan(store);
            const auto deltaColumn =
                static_cast<std::int32_t>(sel.movePointer.column)
                - static_cast<std::int32_t>(sel.moveAnchor.column);
            const auto deltaRow =
                static_cast<std::int32_t>(sel.movePointer.row)
                - static_cast<std::int32_t>(sel.moveAnchor.row);

            if (!span.has_value()
                || (deltaColumn == 0 && deltaRow == 0))
            {
                return;
            }

            moveMapSpan(store.state, *span, deltaColumn, deltaRow);

            const auto columns = static_cast<std::int32_t>(
                store.state.map.columns());
            const auto rows = static_cast<std::int32_t>(
                store.state.map.rows());
            const auto left = std::max(
                static_cast<std::int32_t>(span->origin.column)
                    + deltaColumn,
                0);
            const auto top = std::max(
                static_cast<std::int32_t>(span->origin.row)
                    + deltaRow,
                0);
            const auto right = std::min(
                static_cast<std::int32_t>(span->origin.column)
                    + deltaColumn
                    + static_cast<std::int32_t>(span->columns),
                columns);
            const auto bottom = std::min(
                static_cast<std::int32_t>(span->origin.row)
                    + deltaRow
                    + static_cast<std::int32_t>(span->rows),
                rows);

            if (left >= right || top >= bottom)
            {
                sel.rect.reset();
                return;
            }

            sel.rect = CellSpan{
                .origin =
                    {.column = static_cast<std::uint32_t>(left),
                     .row = static_cast<std::uint32_t>(top)},
                .columns = static_cast<std::uint32_t>(right - left),
                .rows = static_cast<std::uint32_t>(bottom - top)};
        }

        void copyTilesSelection(EditorStore &store)
        {
            const auto span = tilesSelectionSpan(store);

            if (!span.has_value())
            {
                return;
            }

            auto *doc = activeTilesetDoc(store);
            const auto *sprite = activeSpriteOf(*doc);
            const bool present =
                sprite != nullptr
                && doc->sel.frame < sprite->frameCount;
            PixelClipboard clip{
                .width = span->width, .height = span->height};

            for (std::int32_t y = 0; y < span->height; ++y)
            {
                for (std::int32_t x = 0; x < span->width; ++x)
                {
                    const Point at{
                        .x = span->origin.x + x,
                        .y = span->origin.y + y};

                    clip.pixels.push_back(
                        present
                            ? sprite->frames[doc->sel.frame]
                                  .pixels[frameOffsetOf(at)]
                            : PixelClass::Blank);
                }
            }

            store.pixelClipboard = std::move(clip);
        }

        void cutTilesSelection(EditorStore &store)
        {
            copyTilesSelection(store);

            const auto span = tilesSelectionSpan(store);

            if (!span.has_value())
            {
                return;
            }

            auto *doc = activeTilesetDoc(store);
            auto *sprite = activeSpriteOf(*doc);

            if (sprite == nullptr
                || doc->sel.frame >= sprite->frameCount)
            {
                return;
            }

            pushTilesetSnapshot(*doc);

            for (std::int32_t y = 0; y < span->height; ++y)
            {
                for (std::int32_t x = 0; x < span->width; ++x)
                {
                    const Point at{
                        .x = span->origin.x + x,
                        .y = span->origin.y + y};

                    sprite->frames[doc->sel.frame]
                        .pixels[frameOffsetOf(at)] =
                        PixelClass::Blank;
                }
            }

            finishTilesEdit(store, *doc);
        }

        void pasteTilesClipboard(EditorStore &store)
        {
            if (!store.pixelClipboard.has_value()
                || !store.input.canvasPointer.has_value())
            {
                return;
            }

            const auto pixel =
                editorPixelAt(*store.input.canvasPointer);
            auto *doc = activeTilesetDoc(store);

            if (!pixel.has_value() || doc == nullptr)
            {
                return;
            }

            auto *sprite = activeSpriteOf(*doc);

            if (sprite == nullptr)
            {
                return;
            }

            pushTilesetSnapshot(*doc);
            ensureFrame(doc->sel, *sprite);

            const auto &clip = *store.pixelClipboard;
            const auto side =
                static_cast<std::int32_t>(kSpriteSide);

            for (std::int32_t y = 0; y < clip.height; ++y)
            {
                for (std::int32_t x = 0; x < clip.width; ++x)
                {
                    const auto value = clip.pixels
                        [static_cast<std::size_t>(
                            y * clip.width + x)];
                    const Point at{
                        .x = pixel->x + x, .y = pixel->y + y};

                    if (value == PixelClass::Blank
                        || at.x >= side || at.y >= side)
                    {
                        continue;
                    }

                    sprite->frames[doc->sel.frame]
                        .pixels[frameOffsetOf(at)] = value;
                }
            }

            finishTilesEdit(store, *doc);
        }

        void moveTilesSelection(EditorStore &store)
        {
            auto &selection = store.tilesSelection;
            const auto span = tilesSelectionSpan(store);
            const auto delta = Point{
                .x = selection.pixels.movePointer.x
                     - selection.pixels.moveAnchor.x,
                .y = selection.pixels.movePointer.y
                     - selection.pixels.moveAnchor.y};

            if (!span.has_value() || (delta.x == 0 && delta.y == 0))
            {
                return;
            }

            auto *doc = activeTilesetDoc(store);
            auto *sprite = activeSpriteOf(*doc);

            if (sprite != nullptr
                && doc->sel.frame < sprite->frameCount)
            {
                pushTilesetSnapshot(*doc);

                auto &pixels =
                    sprite->frames[doc->sel.frame].pixels;
                std::vector<PixelClass> held{};

                for (std::int32_t y = 0; y < span->height; ++y)
                {
                    for (std::int32_t x = 0; x < span->width; ++x)
                    {
                        const Point at{
                            .x = span->origin.x + x,
                            .y = span->origin.y + y};

                        held.push_back(
                            pixels[frameOffsetOf(at)]);
                        pixels[frameOffsetOf(at)] =
                            PixelClass::Blank;
                    }
                }

                const auto side =
                    static_cast<std::int32_t>(kSpriteSide);

                for (std::int32_t y = 0; y < span->height; ++y)
                {
                    for (std::int32_t x = 0; x < span->width; ++x)
                    {
                        const auto value = held
                            [static_cast<std::size_t>(
                                y * span->width + x)];
                        const Point at{
                            .x = span->origin.x + delta.x + x,
                            .y = span->origin.y + delta.y + y};

                        if (value == PixelClass::Blank || at.x < 0
                            || at.y < 0 || at.x >= side
                            || at.y >= side)
                        {
                            continue;
                        }

                        pixels[frameOffsetOf(at)] = value;
                    }
                }

                finishTilesEdit(store, *doc);
            }

            selection.pixels.rect = clippedPixelSpan(
                Point{
                    .x = span->origin.x + delta.x,
                    .y = span->origin.y + delta.y},
                span->width,
                span->height,
                static_cast<std::int32_t>(kSpriteSide));
        }

        [[nodiscard]] SheetDoc *selectedSheet(EditorStore &store)
        {
            auto &characters = store.characters;

            if (characters.selected >= characters.list.size())
            {
                return nullptr;
            }

            return &characters.list[characters.selected].sheet;
        }

        void copyCharSelection(EditorStore &store)
        {
            const auto span = charSelectionSpan(store);

            if (!span.has_value())
            {
                return;
            }

            auto *doc = selectedSheet(store);
            PixelClipboard clip{
                .width = span->width, .height = span->height};

            for (std::int32_t y = 0; y < span->height; ++y)
            {
                for (std::int32_t x = 0; x < span->width; ++x)
                {
                    clip.pixels.push_back(sheetPixelClass(
                        doc->image,
                        Point{
                            .x = span->origin.x + x,
                            .y = span->origin.y + y}));
                }
            }

            store.pixelClipboard = std::move(clip);
        }

        void cutCharSelection(EditorStore &store)
        {
            copyCharSelection(store);

            const auto span = charSelectionSpan(store);

            if (!span.has_value())
            {
                return;
            }

            auto *doc = selectedSheet(store);

            doc->undoStack.push_back(doc->image);
            doc->redoStack.clear();

            for (std::int32_t y = 0; y < span->height; ++y)
            {
                for (std::int32_t x = 0; x < span->width; ++x)
                {
                    static_cast<void>(setSheetPixel(
                        doc->image,
                        Point{
                            .x = span->origin.x + x,
                            .y = span->origin.y + y},
                        PixelClass::Blank));
                }
            }

            finishSheetEdit(*doc);
        }

        void pasteCharClipboard(EditorStore &store)
        {
            if (!store.pixelClipboard.has_value()
                || !store.input.canvasPointer.has_value())
            {
                return;
            }

            const auto pixel =
                characterPixelAt(*store.input.canvasPointer);
            auto *doc = selectedSheet(store);

            if (!pixel.has_value() || doc == nullptr)
            {
                return;
            }

            doc->undoStack.push_back(doc->image);
            doc->redoStack.clear();

            const auto &clip = *store.pixelClipboard;

            for (std::int32_t y = 0; y < clip.height; ++y)
            {
                for (std::int32_t x = 0; x < clip.width; ++x)
                {
                    const auto value = clip.pixels
                        [static_cast<std::size_t>(
                            y * clip.width + x)];

                    if (value == PixelClass::Blank)
                    {
                        continue;
                    }

                    static_cast<void>(setSheetPixel(
                        doc->image,
                        Point{
                            .x = pixel->x + x, .y = pixel->y + y},
                        value));
                }
            }

            finishSheetEdit(*doc);
        }

        void moveCharSelection(EditorStore &store)
        {
            auto &selection = store.charSelection;
            const auto span = charSelectionSpan(store);
            const auto delta = Point{
                .x = selection.pixels.movePointer.x
                     - selection.pixels.moveAnchor.x,
                .y = selection.pixels.movePointer.y
                     - selection.pixels.moveAnchor.y};

            if (!span.has_value() || (delta.x == 0 && delta.y == 0))
            {
                return;
            }

            auto *doc = selectedSheet(store);

            doc->undoStack.push_back(doc->image);
            doc->redoStack.clear();

            std::vector<PixelClass> held{};

            for (std::int32_t y = 0; y < span->height; ++y)
            {
                for (std::int32_t x = 0; x < span->width; ++x)
                {
                    const Point at{
                        .x = span->origin.x + x,
                        .y = span->origin.y + y};

                    held.push_back(
                        sheetPixelClass(doc->image, at));
                    static_cast<void>(setSheetPixel(
                        doc->image, at, PixelClass::Blank));
                }
            }

            for (std::int32_t y = 0; y < span->height; ++y)
            {
                for (std::int32_t x = 0; x < span->width; ++x)
                {
                    const auto value = held
                        [static_cast<std::size_t>(
                            y * span->width + x)];

                    if (value == PixelClass::Blank)
                    {
                        continue;
                    }

                    static_cast<void>(setSheetPixel(
                        doc->image,
                        Point{
                            .x = span->origin.x + delta.x + x,
                            .y = span->origin.y + delta.y + y},
                        value));
                }
            }

            finishSheetEdit(*doc);

            selection.pixels.rect = clippedPixelSpan(
                Point{
                    .x = span->origin.x + delta.x,
                    .y = span->origin.y + delta.y},
                span->width,
                span->height,
                static_cast<std::int32_t>(kCharacterSize));
        }

        void drawDashedRect(
            gfx::ViewportRenderer &view,
            const RectF rect,
            const Color color)
        {
            const auto right = rect.origin.x + rect.size.width;
            const auto bottom = rect.origin.y + rect.size.height;

            for (auto x = rect.origin.x; x < right;
                 x += kDashPeriod)
            {
                const auto width = std::min(kDashOn, right - x);

                view.drawRect(
                    RectF({x, rect.origin.y}, {width, 1.0F}),
                    color);
                view.drawRect(
                    RectF({x, bottom - 1.0F}, {width, 1.0F}),
                    color);
            }

            for (auto y = rect.origin.y; y < bottom;
                 y += kDashPeriod)
            {
                const auto height = std::min(kDashOn, bottom - y);

                view.drawRect(
                    RectF({rect.origin.x, y}, {1.0F, height}),
                    color);
                view.drawRect(
                    RectF({right - 1.0F, y}, {1.0F, height}),
                    color);
            }
        }

        void drawSolidRect(
            gfx::ViewportRenderer &view,
            const RectF rect,
            const Color color)
        {
            view.drawRect(
                RectF(rect.origin, {rect.size.width, 1.0F}),
                color);
            view.drawRect(
                RectF(
                    {rect.origin.x,
                     rect.origin.y + rect.size.height - 1.0F},
                    {rect.size.width, 1.0F}),
                color);
            view.drawRect(
                RectF(
                    {rect.origin.x, rect.origin.y + 1.0F},
                    {1.0F, rect.size.height - 2.0F}),
                color);
            view.drawRect(
                RectF(
                    {rect.origin.x + rect.size.width - 1.0F,
                     rect.origin.y + 1.0F},
                    {1.0F, rect.size.height - 2.0F}),
                color);
        }

        [[nodiscard]] RectF pixelSpanRect(
            const PixelSpan &span,
            const float left,
            const float top,
            const float zoom)
        {
            return RectF(
                {left + static_cast<float>(span.origin.x) * zoom,
                 top + static_cast<float>(span.origin.y) * zoom},
                {static_cast<float>(span.width) * zoom,
                 static_cast<float>(span.height) * zoom});
        }

        void drawPixelSelection(
            gfx::ViewportRenderer &view,
            const PixelSelection &sel,
            const std::optional<PixelSpan> &placed,
            const float left,
            const float top,
            const float zoom)
        {
            if (sel.dragging)
            {
                drawDashedRect(
                    view,
                    pixelSpanRect(
                        pixelSpanOf(sel.anchor, sel.focus),
                        left,
                        top,
                        zoom),
                    kMarqueeColor);
                return;
            }

            if (!placed.has_value())
            {
                return;
            }

            if (sel.moving)
            {
                auto rect =
                    pixelSpanRect(*placed, left, top, zoom);

                rect.origin.x +=
                    static_cast<float>(
                        sel.movePointer.x - sel.moveAnchor.x)
                    * zoom;
                rect.origin.y +=
                    static_cast<float>(
                        sel.movePointer.y - sel.moveAnchor.y)
                    * zoom;
                drawDashedRect(view, rect, kMarqueeColor);
                return;
            }

            drawSolidRect(
                view,
                pixelSpanRect(*placed, left, top, zoom),
                kMarqueeColor);
        }
    }

    CellSpan cellSpanOf(
        const geometry::GridCell a, const geometry::GridCell b)
    {
        const auto left = std::min(a.column, b.column);
        const auto right = std::max(a.column, b.column);
        const auto top = std::min(a.row, b.row);
        const auto bottom = std::max(a.row, b.row);

        return CellSpan{
            .origin = {.column = left, .row = top},
            .columns = right - left + 1,
            .rows = bottom - top + 1};
    }

    PixelSpan pixelSpanOf(const gfx::Point a, const gfx::Point b)
    {
        const auto left = std::min(a.x, b.x);
        const auto right = std::max(a.x, b.x);
        const auto top = std::min(a.y, b.y);
        const auto bottom = std::max(a.y, b.y);

        return PixelSpan{
            .origin = {.x = left, .y = top},
            .width = right - left + 1,
            .height = bottom - top + 1};
    }

    bool cellSpanContains(
        const CellSpan &span, const geometry::GridCell cell)
    {
        return cell.column >= span.origin.column
               && cell.column < span.origin.column + span.columns
               && cell.row >= span.origin.row
               && cell.row < span.origin.row + span.rows;
    }

    bool pixelSpanContains(
        const PixelSpan &span, const gfx::Point pixel)
    {
        return pixel.x >= span.origin.x
               && pixel.x < span.origin.x + span.width
               && pixel.y >= span.origin.y
               && pixel.y < span.origin.y + span.height;
    }

    std::optional<CellSpan> mapSelectionSpan(
        const EditorStore &store)
    {
        const auto &rect = store.mapSelection.rect;

        if (!rect.has_value())
        {
            return std::nullopt;
        }

        const auto columns = store.state.map.columns();
        const auto rows = store.state.map.rows();

        if (rect->origin.column >= columns
            || rect->origin.row >= rows)
        {
            return std::nullopt;
        }

        auto span = *rect;

        span.columns = std::min(
            span.columns, columns - span.origin.column);
        span.rows = std::min(span.rows, rows - span.origin.row);

        return span;
    }

    std::optional<PixelSpan> tilesSelectionSpan(
        const EditorStore &store)
    {
        const auto &selection = store.tilesSelection;
        const auto *doc = activeTilesetDoc(store);

        if (!selection.pixels.rect.has_value() || doc == nullptr
            || selection.doc != store.tilesets.active
            || !(selection.ctx == doc->sel))
        {
            return std::nullopt;
        }

        return selection.pixels.rect;
    }

    std::optional<PixelSpan> charSelectionSpan(
        const EditorStore &store)
    {
        const auto &selection = store.charSelection;

        if (!selection.pixels.rect.has_value()
            || selection.character != store.characters.selected
            || store.characters.selected
                   >= store.characters.list.size())
        {
            return std::nullopt;
        }

        return selection.pixels.rect;
    }

    void applyMapSelectGesture(
        EditorStore &store, const MapGesture &gesture)
    {
        auto &sel = store.mapSelection;

        if (gesture.erase)
        {
            return;
        }

        if (gesture.kind == GestureKind::Press)
        {
            const auto span = mapSelectionSpan(store);

            if (span.has_value()
                && cellSpanContains(*span, gesture.cell))
            {
                sel.moving = true;
                sel.moveAnchor = gesture.cell;
                sel.movePointer = gesture.cell;
                return;
            }

            sel = MapSelection{};
            sel.dragging = true;
            sel.anchor = gesture.cell;
            sel.focus = gesture.cell;
            return;
        }

        if (gesture.kind == GestureKind::Move)
        {
            if (sel.moving)
            {
                sel.movePointer = gesture.cell;
                return;
            }

            if (sel.dragging)
            {
                if (!(gesture.cell == sel.anchor))
                {
                    sel.dragged = true;
                }

                sel.focus = gesture.cell;
            }

            return;
        }

        if (sel.moving)
        {
            sel.moving = false;
            finishMapMove(store);
            return;
        }

        if (!sel.dragging)
        {
            return;
        }

        sel.dragging = false;

        if (!sel.dragged)
        {
            sel.rect.reset();
            return;
        }

        sel.rect = cellSpanOf(sel.anchor, sel.focus);
    }

    void applyTilesSelectGesture(
        EditorStore &store, const SheetGesture &gesture)
    {
        auto &selection = store.tilesSelection;
        auto &sel = selection.pixels;
        auto *doc = activeTilesetDoc(store);

        if (doc == nullptr)
        {
            return;
        }

        if (gesture.kind == GestureKind::Press)
        {
            if (!gesture.ink)
            {
                return;
            }

            const auto pixel = editorPixelAt(gesture.pixel);

            if (!pixel.has_value())
            {
                return;
            }

            const auto span = tilesSelectionSpan(store);

            if (span.has_value()
                && pixelSpanContains(*span, *pixel))
            {
                sel.moving = true;
                sel.moveAnchor = *pixel;
                sel.movePointer = *pixel;
                return;
            }

            sel = PixelSelection{};
            sel.dragging = true;
            sel.anchor = *pixel;
            sel.focus = *pixel;
            selection.doc = store.tilesets.active;
            selection.ctx = doc->sel;
            return;
        }

        if (gesture.kind == GestureKind::Move)
        {
            const auto pixel = clampedEditorPixel(gesture.pixel);

            if (sel.moving)
            {
                sel.movePointer = pixel;
                return;
            }

            if (sel.dragging)
            {
                if (pixel.x != sel.anchor.x
                    || pixel.y != sel.anchor.y)
                {
                    sel.dragged = true;
                }

                sel.focus = pixel;
            }

            return;
        }

        if (sel.moving)
        {
            sel.moving = false;
            moveTilesSelection(store);
            return;
        }

        if (!sel.dragging)
        {
            return;
        }

        sel.dragging = false;

        if (!sel.dragged)
        {
            sel.rect.reset();
            return;
        }

        sel.rect = pixelSpanOf(sel.anchor, sel.focus);
    }

    void applyCharSelectGesture(
        EditorStore &store, const SheetGesture &gesture)
    {
        auto &selection = store.charSelection;
        auto &sel = selection.pixels;

        if (selectedSheet(store) == nullptr)
        {
            return;
        }

        if (gesture.kind == GestureKind::Press)
        {
            if (!gesture.ink)
            {
                return;
            }

            const auto span = charSelectionSpan(store);

            if (span.has_value()
                && pixelSpanContains(*span, gesture.pixel))
            {
                sel.moving = true;
                sel.moveAnchor = gesture.pixel;
                sel.movePointer = gesture.pixel;
                return;
            }

            sel = PixelSelection{};
            sel.dragging = true;
            sel.anchor = gesture.pixel;
            sel.focus = gesture.pixel;
            selection.character = store.characters.selected;
            return;
        }

        if (gesture.kind == GestureKind::Move)
        {
            if (sel.moving)
            {
                sel.movePointer = gesture.pixel;
                return;
            }

            if (sel.dragging)
            {
                if (gesture.pixel.x != sel.anchor.x
                    || gesture.pixel.y != sel.anchor.y)
                {
                    sel.dragged = true;
                }

                sel.focus = gesture.pixel;
            }

            return;
        }

        if (sel.moving)
        {
            sel.moving = false;
            moveCharSelection(store);
            return;
        }

        if (!sel.dragging)
        {
            return;
        }

        sel.dragging = false;

        if (!sel.dragged)
        {
            sel.rect.reset();
            return;
        }

        sel.rect = pixelSpanOf(sel.anchor, sel.focus);
    }

    bool selectionChord(EditorStore &store, const input::Key key)
    {
        switch (key)
        {
            case input::Key::C:
                copySelection(store);
                return true;
            case input::Key::X:
                cutSelection(store);
                return true;
            case input::Key::V:
                pasteClipboard(store);
                return true;
            default:
                return false;
        }
    }

    void copySelection(EditorStore &store)
    {
        switch (store.view)
        {
            case EditorView::Map:
                copyMapSelection(store);
                return;
            case EditorView::Tiles:
                copyTilesSelection(store);
                return;
            default:
                copyCharSelection(store);
                return;
        }
    }

    void cutSelection(EditorStore &store)
    {
        switch (store.view)
        {
            case EditorView::Map:
                cutMapSelection(store);
                return;
            case EditorView::Tiles:
                cutTilesSelection(store);
                return;
            default:
                cutCharSelection(store);
                return;
        }
    }

    void pasteClipboard(EditorStore &store)
    {
        switch (store.view)
        {
            case EditorView::Map:
                pasteMapClipboard(store);
                return;
            case EditorView::Tiles:
                pasteTilesClipboard(store);
                return;
            default:
                pasteCharClipboard(store);
                return;
        }
    }

    bool clearActiveSelection(EditorStore &store)
    {
        if (store.view == EditorView::Map)
        {
            const bool live =
                mapSelectionSpan(store).has_value()
                || store.mapSelection.dragging
                || store.mapSelection.moving;

            store.mapSelection = MapSelection{};

            return live;
        }

        if (store.view == EditorView::Tiles)
        {
            const bool live =
                tilesSelectionSpan(store).has_value()
                || store.tilesSelection.pixels.dragging
                || store.tilesSelection.pixels.moving;

            store.tilesSelection = TilesSelection{};

            return live;
        }

        const bool live =
            charSelectionSpan(store).has_value()
            || store.charSelection.pixels.dragging
            || store.charSelection.pixels.moving;

        store.charSelection = CharacterSelection{};

        return live;
    }

    bool exitActiveSelectTool(EditorStore &store)
    {
        if (store.view == EditorView::Map)
        {
            if (store.mapTool != MapTool::Select)
            {
                return false;
            }

            store.mapTool = MapTool::Paint;

            return true;
        }

        if (store.view == EditorView::Tiles)
        {
            if (store.tilesets.tool != TilesetTool::Select)
            {
                return false;
            }

            store.tilesets.tool = TilesetTool::Draw;

            return true;
        }

        if (store.characters.tool != CharacterTool::Select)
        {
            return false;
        }

        store.characters.tool = CharacterTool::Draw;

        return true;
    }

    void clearSelectionsAfterHistory(EditorStore &store)
    {
        store.mapSelection = MapSelection{};
        store.tilesSelection = TilesSelection{};
        store.charSelection = CharacterSelection{};
    }

    void drawTilesSelectionOverlay(
        gfx::ViewportRenderer &view, const EditorStore &store)
    {
        drawPixelSelection(
            view,
            store.tilesSelection.pixels,
            tilesSelectionSpan(store),
            static_cast<float>(kTilesetEditorLeft),
            static_cast<float>(kTilesetEditorTop),
            static_cast<float>(kTilesetEditorZoom));
    }

    void drawCharSelectionOverlay(
        gfx::ViewportRenderer &view, const EditorStore &store)
    {
        drawPixelSelection(
            view,
            store.charSelection.pixels,
            charSelectionSpan(store),
            static_cast<float>(kCharacterLeft),
            static_cast<float>(kCharacterTop),
            static_cast<float>(kCharacterZoom));
    }

}
