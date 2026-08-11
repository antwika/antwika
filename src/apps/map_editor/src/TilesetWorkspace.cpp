#include "antwika/map_editor/TilesetWorkspace.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <antwika/enums/Enumeration.hpp>
#include <antwika/geometry/Rect.hpp>
#include <antwika/gfx/Glyphs.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/tilemap/TerrainClass.hpp>
#include <antwika/tileset/Tileset.hpp>
#include <antwika/tileset/TilesetError.hpp>
#include <antwika/tileset/TilesetFile.hpp>
#include <antwika/ui/Theme.hpp>

#include "antwika/map_editor/Selection.hpp"
#include "antwika/map_editor/SheetWorkspace.hpp"

namespace antwika::map_editor
{

    namespace
    {
        using antwika::gfx::Color;
        using antwika::gfx::Point;
        using antwika::gfx::PointF;
        using antwika::gfx::RectF;
        using antwika::tileset::kEdgeSocket;
        using antwika::tileset::kOpenSocket;
        using antwika::tileset::kSpriteSide;
        using antwika::tileset::PixelClass;
        using antwika::tileset::Side;
        using antwika::tileset::SocketId;
        using antwika::tileset::Sprite;

        constexpr std::size_t kUndoDepth = 64;

        constexpr std::int32_t kEditorExtent =
            kSpriteSide * kTilesetEditorZoom;

        constexpr std::int32_t kBandThickness = 8;

        constexpr std::int32_t kFrameStripTop = 190;

        constexpr std::int32_t kFramePreviewSize = 24;

        constexpr std::int32_t kFramePreviewPitch = 28;

        constexpr std::int32_t kAnimPreviewLeft = 140;

        constexpr std::int32_t kLibraryArt = 16;

        constexpr std::int32_t kPreviewLeft = 8;

        constexpr std::int32_t kPreviewTop = 218;

        constexpr std::uint32_t kPreviewFramePeriod = 30;

        constexpr Color kBackdropLight{
            .red = 74, .green = 76, .blue = 84};

        constexpr Color kBackdropDark{
            .red = 58, .green = 60, .blue = 66};

        constexpr Color kDimBackdrop{
            .red = 40, .green = 42, .blue = 48};

        constexpr Color kPixelGridColor{
            .red = 110, .green = 114, .blue = 124, .alpha = 70};

        constexpr Color kLabelColor{
            .red = 214, .green = 224, .blue = 216};

        constexpr Color kAllowedBackdrop{
            .red = 46, .green = 96, .blue = 60};

        constexpr Color kDimOverlay{
            .red = 12, .green = 14, .blue = 16, .alpha = 150};

        constexpr Color kWhite{
            .red = 255, .green = 255, .blue = 255};

        constexpr Color kOpenColor{
            .red = 70, .green = 74, .blue = 82};

        constexpr Color kEdgeBase{};

        constexpr std::array<Color, kMaxNamedSockets> kSocketCycle{
            Color{.red = 226, .green = 84, .blue = 84},
            Color{.red = 230, .green = 150, .blue = 60},
            Color{.red = 226, .green = 216, .blue = 84},
            Color{.red = 140, .green = 216, .blue = 80},
            Color{.red = 70, .green = 200, .blue = 120},
            Color{.red = 70, .green = 206, .blue = 206},
            Color{.red = 84, .green = 150, .blue = 226},
            Color{.red = 110, .green = 92, .blue = 226},
            Color{.red = 176, .green = 92, .blue = 226},
            Color{.red = 226, .green = 92, .blue = 176},
            Color{.red = 152, .green = 120, .blue = 84},
            Color{.red = 160, .green = 170, .blue = 180}};

        struct BandRect final
        {
            std::int32_t x = 0;
            std::int32_t y = 0;
            std::int32_t width = 0;
            std::int32_t height = 0;
        };

        [[nodiscard]] constexpr BandRect bandRect(
            const Side side) noexcept
        {
            switch (side)
            {
                case Side::North:
                    return {
                        .x = kTilesetEditorLeft,
                        .y = kTilesetEditorTop - kBandThickness - 4,
                        .width = kEditorExtent,
                        .height = kBandThickness};
                case Side::South:
                    return {
                        .x = kTilesetEditorLeft,
                        .y = kTilesetEditorTop + kEditorExtent + 4,
                        .width = kEditorExtent,
                        .height = kBandThickness};
                case Side::West:
                    return {
                        .x = kTilesetEditorLeft - kBandThickness - 4,
                        .y = kTilesetEditorTop,
                        .width = kBandThickness,
                        .height = kEditorExtent};
                default:
                    return {
                        .x = kTilesetEditorLeft + kEditorExtent + 4,
                        .y = kTilesetEditorTop,
                        .width = kBandThickness,
                        .height = kEditorExtent};
            }
        }

        [[nodiscard]] bool insideBand(
            const BandRect &band, const Point canvas) noexcept
        {
            return canvas.x >= band.x
                   && canvas.x < band.x + band.width
                   && canvas.y >= band.y
                   && canvas.y < band.y + band.height;
        }

        constexpr BandRect kPreviewRect{
            .x = kPreviewLeft,
            .y = kPreviewTop,
            .width = kPreviewColumns * kSpriteSide,
            .height = kPreviewRows * kSpriteSide};

        constexpr BandRect kRegenRect{
            .x = 104, .y = 220, .width = 34, .height = 11};

        constexpr BandRect kAutoRect{
            .x = 104, .y = 238, .width = 10, .height = 10};

        void clampSelection(TilesetDoc &doc)
        {
            auto &sel = doc.sel;

            if (doc.data.layers.empty())
            {
                sel = {};
                return;
            }

            sel.layer =
                std::min(sel.layer, doc.data.layers.size() - 1);

            const auto sprites =
                doc.data.layers[sel.layer].sprites.size();

            sel.sprite =
                sprites == 0 ? 0 : std::min(sel.sprite, sprites - 1);
            sel.frame = std::min<std::size_t>(
                sel.frame, tileset::kMaxFrames - 1);
        }

        void mutated(EditorStore &store, TilesetDoc &doc)
        {
            doc.dirty = true;
            ++doc.revision;
            store.tilesets.message.clear();
        }

        [[nodiscard]] Sprite *selectedSprite(TilesetDoc &doc)
        {
            clampSelection(doc);

            auto &sprites = doc.data.layers[doc.sel.layer].sprites;

            if (sprites.empty())
            {
                return nullptr;
            }

            return &sprites[doc.sel.sprite];
        }

        [[nodiscard]] const Sprite *selectedSprite(
            const TilesetDoc &doc)
        {
            const auto &sel = doc.sel;

            if (sel.layer >= doc.data.layers.size())
            {
                return nullptr;
            }

            const auto &sprites =
                doc.data.layers[sel.layer].sprites;

            if (sel.sprite >= sprites.size())
            {
                return nullptr;
            }

            return &sprites[sel.sprite];
        }

        [[nodiscard]] bool decorMode(
            const TilesetWorkspace &tilesets, const TilesetDoc &doc)
        {
            return tilesets.tool == TilesetTool::Decor
                   && doc.sel.layer >= 1;
        }

        [[nodiscard]] std::size_t lastLayerIndex(
            const TilesetDoc &doc) noexcept
        {
            return doc.data.layers.size() - 1;
        }

        [[nodiscard]] std::size_t libraryPageCount(
            const TilesetWorkspace &tilesets, const TilesetDoc &doc)
        {
            const bool decor = decorMode(tilesets, doc);
            const auto shown =
                decor ? doc.data.layers[0].sprites.size()
                      : doc.data.layers[doc.sel.layer].sprites.size()
                            + 1;

            return std::max<std::size_t>(
                1,
                (shown + kLibraryPageSize - 1) / kLibraryPageSize);
        }

        void ensureSelectedFrame(
            EditorStore &store, TilesetDoc &doc, Sprite &sprite)
        {
            if (doc.sel.frame < sprite.frameCount)
            {
                return;
            }

            for (auto frame = sprite.frameCount;
                 frame <= doc.sel.frame;
                 ++frame)
            {
                sprite.frames[frame] = sprite.frames[0];
            }

            sprite.frameCount =
                static_cast<std::uint8_t>(doc.sel.frame + 1);
            mutated(store, doc);
        }

        void paintPixel(
            EditorStore &store,
            TilesetDoc &doc,
            const Point pixel,
            const bool ink)
        {
            auto *sprite = selectedSprite(doc);

            if (sprite == nullptr)
            {
                return;
            }

            const auto value =
                !ink ? PixelClass::Blank
                     : (store.tilesets.drawPaper ? PixelClass::Paper
                                                 : PixelClass::Ink);
            auto &held = sprite->frames[doc.sel.frame].pixels
                [static_cast<std::size_t>(
                    pixel.y * kSpriteSide + pixel.x)];

            if (held == value)
            {
                return;
            }

            held = value;
            mutated(store, doc);
        }

        [[nodiscard]] bool setDecorAllowed(
            Sprite &sprite,
            const tileset::SpriteId base,
            const bool allowed)
        {
            const auto found = std::ranges::find(sprite.on, base);

            if (allowed == (found != sprite.on.end()))
            {
                return false;
            }

            if (allowed)
            {
                sprite.on.push_back(base);
                std::ranges::sort(sprite.on);
                return true;
            }

            sprite.on.erase(found);
            return true;
        }

        void applyDecorCell(
            EditorStore &store,
            TilesetDoc &doc,
            const std::size_t cell,
            const bool allowed)
        {
            const auto &base = doc.data.layers[0].sprites;
            const auto at =
                store.tilesets.libraryPage * kLibraryPageSize + cell;
            auto *sprite = selectedSprite(doc);

            if (sprite == nullptr || at >= base.size())
            {
                return;
            }

            if (setDecorAllowed(*sprite, base[at].id, allowed))
            {
                mutated(store, doc);
            }
        }

        void endStroke(EditorStore &store, TilesetDoc &doc)
        {
            auto &tilesets = store.tilesets;

            if (!tilesets.stroke)
            {
                return;
            }

            tilesets.stroke = false;
            tilesets.decorStroke = false;

            if (!doc.undoStack.empty()
                && doc.undoStack.back().data == doc.data)
            {
                doc.undoStack.pop_back();
            }
        }

        void pressLibrary(
            EditorStore &store,
            TilesetDoc &doc,
            const std::size_t cell,
            const bool ink)
        {
            auto &tilesets = store.tilesets;

            if (!ink)
            {
                return;
            }

            const auto at =
                tilesets.libraryPage * kLibraryPageSize + cell;

            if (decorMode(tilesets, doc))
            {
                const auto &base = doc.data.layers[0].sprites;
                auto *sprite = selectedSprite(doc);

                if (sprite == nullptr || at >= base.size())
                {
                    return;
                }

                pushTilesetSnapshot(doc);

                const auto &on = sprite->on;
                const bool allowed =
                    std::ranges::find(on, base[at].id) == on.end();

                tilesets.stroke = true;
                tilesets.decorStroke = true;
                tilesets.strokeInk = allowed;
                applyDecorCell(store, doc, cell, allowed);
                return;
            }

            const auto &sprites =
                doc.data.layers[doc.sel.layer].sprites;

            if (at < sprites.size())
            {
                doc.sel.sprite = at;
                tilesets.message.clear();
                return;
            }

            if (at == sprites.size())
            {
                addSpritePressed(store);
            }
        }

        void pressBand(
            EditorStore &store,
            TilesetDoc &doc,
            const Side side,
            const bool ink)
        {
            auto &tilesets = store.tilesets;

            if (tilesets.tool != TilesetTool::Sockets)
            {
                return;
            }

            auto *sprite = selectedSprite(doc);

            if (sprite == nullptr)
            {
                return;
            }

            auto &slot = sprite->sockets[enums::index(side)];
            auto want = kOpenSocket;

            if (ink)
            {
                if (!tilesets.activeSocket.has_value())
                {
                    tilesets.message = "pick a socket";
                    return;
                }

                const auto active = static_cast<SocketId>(
                    *tilesets.activeSocket);

                want = slot == active ? kOpenSocket : active;
            }

            if (slot == want)
            {
                return;
            }

            pushTilesetSnapshot(doc);
            slot = want;
            mutated(store, doc);
        }

        void beginPixelStroke(
            EditorStore &store,
            TilesetDoc &doc,
            const Point pixel,
            const bool ink)
        {
            auto *sprite = selectedSprite(doc);

            if (sprite == nullptr)
            {
                return;
            }

            pushTilesetSnapshot(doc);
            store.tilesets.stroke = true;
            store.tilesets.decorStroke = false;
            store.tilesets.strokeInk = ink;
            ensureSelectedFrame(store, doc, *sprite);
            paintPixel(store, doc, pixel, ink);
        }

        void continueStroke(
            EditorStore &store,
            TilesetDoc &doc,
            const Point canvas)
        {
            auto &tilesets = store.tilesets;

            if (!tilesets.stroke)
            {
                return;
            }

            if (tilesets.decorStroke)
            {
                if (const auto cell = libraryCellAt(canvas))
                {
                    applyDecorCell(
                        store, doc, *cell, tilesets.strokeInk);
                }

                return;
            }

            if (const auto pixel = editorPixelAt(canvas))
            {
                paintPixel(store, doc, *pixel, tilesets.strokeInk);
            }
        }

        [[nodiscard]] bool socketInUse(
            const tileset::Tileset &data, const SocketId socket)
        {
            for (const auto &layer : data.layers)
            {
                for (const auto &sprite : layer.sprites)
                {
                    for (const auto held : sprite.sockets)
                    {
                        if (held == socket)
                        {
                            return true;
                        }
                    }
                }
            }

            return false;
        }

        [[nodiscard]] bool socketNameTaken(
            const tileset::Tileset &data, const std::string &name)
        {
            return std::ranges::find(data.socketNames, name)
                   != data.socketNames.end();
        }
    }

    std::optional<Point> editorPixelAt(const Point canvas) noexcept
    {
        const auto localX = canvas.x - kTilesetEditorLeft;
        const auto localY = canvas.y - kTilesetEditorTop;

        if (localX < 0 || localY < 0 || localX >= kEditorExtent
            || localY >= kEditorExtent)
        {
            return std::nullopt;
        }

        return Point{
            .x = localX / kTilesetEditorZoom,
            .y = localY / kTilesetEditorZoom};
    }

    std::optional<Side> socketBandAt(const Point canvas) noexcept
    {
        for (const auto side : enums::kAll<Side>)
        {
            if (insideBand(bandRect(side), canvas))
            {
                return side;
            }
        }

        return std::nullopt;
    }

    std::optional<std::size_t> libraryCellAt(
        const Point canvas) noexcept
    {
        const auto localX = canvas.x - kLibraryLeft;
        const auto localY = canvas.y - kLibraryTop;

        if (localX < 0 || localY < 0)
        {
            return std::nullopt;
        }

        const auto column = localX / kLibraryPitch;
        const auto row = localY / kLibraryPitch;

        if (column >= static_cast<std::int32_t>(kLibraryColumns)
            || row >= static_cast<std::int32_t>(kLibraryRows)
            || localX % kLibraryPitch >= kLibraryArt
            || localY % kLibraryPitch >= kLibraryArt)
        {
            return std::nullopt;
        }

        return static_cast<std::size_t>(row) * kLibraryColumns
               + static_cast<std::size_t>(column);
    }

    std::optional<std::size_t> framePreviewAt(
        const Point canvas) noexcept
    {
        if (canvas.y < kFrameStripTop
            || canvas.y >= kFrameStripTop + kFramePreviewSize)
        {
            return std::nullopt;
        }

        for (std::size_t frame = 0;
             frame < tileset::kMaxFrames;
             ++frame)
        {
            const auto left =
                kTilesetEditorLeft
                + static_cast<std::int32_t>(frame)
                      * kFramePreviewPitch;

            if (canvas.x >= left
                && canvas.x < left + kFramePreviewSize)
            {
                return frame;
            }
        }

        return std::nullopt;
    }

    bool overLibrary(const Point canvas) noexcept
    {
        return canvas.x >= kLibraryLeft
               && canvas.x < kLibraryLeft
                                 + static_cast<std::int32_t>(
                                       kLibraryColumns)
                                       * kLibraryPitch
               && canvas.y >= kLibraryTop
               && canvas.y < kLibraryTop
                                 + static_cast<std::int32_t>(
                                       kLibraryRows)
                                       * kLibraryPitch;
    }

    bool overPreview(const Point canvas) noexcept
    {
        return insideBand(kPreviewRect, canvas);
    }

    bool overPreviewRegen(const Point canvas) noexcept
    {
        return insideBand(kRegenRect, canvas);
    }

    bool overPreviewAuto(const Point canvas) noexcept
    {
        return insideBand(kAutoRect, canvas);
    }

    Color socketColor(const SocketId socket) noexcept
    {
        if (socket == kEdgeSocket)
        {
            return kEdgeBase;
        }

        if (socket == kOpenSocket)
        {
            return kOpenColor;
        }

        return kSocketCycle[(socket - 2U) % kMaxNamedSockets];
    }

    void adjustLibraryPage(
        EditorStore &store, const std::int32_t delta)
    {
        const auto *doc = activeTilesetDoc(store);

        if (doc == nullptr)
        {
            return;
        }

        const auto pages =
            libraryPageCount(store.tilesets, *doc);
        const auto page = static_cast<std::int64_t>(
                              store.tilesets.libraryPage)
                          + delta;

        store.tilesets.libraryPage = static_cast<std::size_t>(
            std::clamp<std::int64_t>(
                page, 0, static_cast<std::int64_t>(pages) - 1));
    }

    void activateTileset(EditorStore &store, const std::size_t index)
    {
        auto &tilesets = store.tilesets;

        if (index >= tilesets.open.size())
        {
            return;
        }

        tilesets.active = index;
        tilesets.libraryPage = 0;
        tilesets.activeSocket.reset();
        tilesets.confirmDeleteSprite = false;
        tilesets.message.clear();
    }

    void pushTilesetSnapshot(TilesetDoc &doc)
    {
        if (doc.undoStack.size() >= kUndoDepth)
        {
            doc.undoStack.erase(doc.undoStack.begin());
        }

        TilesetSnapshot snapshot;

        snapshot.data = doc.data;
        snapshot.sel = doc.sel;
        doc.undoStack.push_back(std::move(snapshot));
        doc.redoStack.clear();
    }

    void applyTilesetGesture(
        EditorStore &store, const SheetGesture &gesture)
    {
        auto *doc = activeTilesetDoc(store);

        if (doc == nullptr)
        {
            return;
        }

        const bool selecting =
            store.tilesets.tool == TilesetTool::Select;

        if (gesture.kind == GestureKind::Release)
        {
            if (selecting)
            {
                applyTilesSelectGesture(store, gesture);
                return;
            }

            endStroke(store, *doc);
            return;
        }

        if (gesture.kind == GestureKind::Move)
        {
            if (selecting)
            {
                applyTilesSelectGesture(store, gesture);
                return;
            }

            continueStroke(store, *doc, gesture.pixel);
            return;
        }

        store.tilesets.confirmDeleteSprite = false;

        if (const auto frame = framePreviewAt(gesture.pixel))
        {
            selectTilesetFrame(store, *frame);
            return;
        }

        if (overPreviewRegen(gesture.pixel))
        {
            if (gesture.ink)
            {
                ++store.tilesets.previewSeed;
            }

            return;
        }

        if (overPreviewAuto(gesture.pixel))
        {
            if (gesture.ink)
            {
                store.tilesets.previewAuto =
                    !store.tilesets.previewAuto;
            }

            return;
        }

        if (const auto cell = libraryCellAt(gesture.pixel))
        {
            pressLibrary(store, *doc, *cell, gesture.ink);
            return;
        }

        if (const auto side = socketBandAt(gesture.pixel))
        {
            pressBand(store, *doc, *side, gesture.ink);
            return;
        }

        if (const auto pixel = editorPixelAt(gesture.pixel))
        {
            if (selecting)
            {
                applyTilesSelectGesture(store, gesture);
                return;
            }

            beginPixelStroke(store, *doc, *pixel, gesture.ink);
        }
    }

    void tilesetUndo(EditorStore &store)
    {
        auto *doc = activeTilesetDoc(store);

        if (doc == nullptr || doc->undoStack.empty())
        {
            return;
        }

        if (doc->redoStack.size() >= kUndoDepth)
        {
            doc->redoStack.erase(doc->redoStack.begin());
        }

        TilesetSnapshot snapshot;

        snapshot.data = doc->data;
        snapshot.sel = doc->sel;
        doc->redoStack.push_back(std::move(snapshot));
        doc->data = std::move(doc->undoStack.back().data);
        doc->sel = doc->undoStack.back().sel;
        doc->undoStack.pop_back();
        clampSelection(*doc);
        doc->dirty = true;
        ++doc->revision;
    }

    void tilesetRedo(EditorStore &store)
    {
        auto *doc = activeTilesetDoc(store);

        if (doc == nullptr || doc->redoStack.empty())
        {
            return;
        }

        if (doc->undoStack.size() >= kUndoDepth)
        {
            doc->undoStack.erase(doc->undoStack.begin());
        }

        TilesetSnapshot snapshot;

        snapshot.data = doc->data;
        snapshot.sel = doc->sel;
        doc->undoStack.push_back(std::move(snapshot));
        doc->data = std::move(doc->redoStack.back().data);
        doc->sel = doc->redoStack.back().sel;
        doc->redoStack.pop_back();
        clampSelection(*doc);
        doc->dirty = true;
        ++doc->revision;
    }

    void selectTilesetFrame(
        EditorStore &store, const std::size_t frame)
    {
        auto *doc = activeTilesetDoc(store);

        if (doc == nullptr || frame >= tileset::kMaxFrames)
        {
            return;
        }

        doc->sel.frame = frame;
    }

    void clearActiveFrame(EditorStore &store)
    {
        auto *doc = activeTilesetDoc(store);

        if (doc == nullptr)
        {
            return;
        }

        auto *sprite = selectedSprite(*doc);

        if (sprite == nullptr)
        {
            return;
        }

        const auto frame = doc->sel.frame;

        if (frame == 0)
        {
            if (sprite->frames[0] == tileset::SpriteFrame{})
            {
                return;
            }

            pushTilesetSnapshot(*doc);
            sprite->frames[0] = {};
        }
        else
        {
            if (sprite->frameCount <= frame)
            {
                return;
            }

            pushTilesetSnapshot(*doc);
            sprite->frameCount =
                static_cast<std::uint8_t>(frame);

            for (auto trailing = frame;
                 trailing < tileset::kMaxFrames;
                 ++trailing)
            {
                sprite->frames[trailing] = {};
            }

            doc->sel.frame = frame - 1;
        }

        mutated(store, *doc);
    }

    void addLayerPressed(EditorStore &store)
    {
        auto *doc = activeTilesetDoc(store);

        if (doc == nullptr)
        {
            return;
        }

        pushTilesetSnapshot(*doc);
        static_cast<void>(tileset::addLayer(
            doc->data,
            "decor" + std::to_string(doc->data.layers.size())));
        doc->sel.layer = doc->data.layers.size() - 1;
        doc->sel.sprite = 0;
        store.tilesets.libraryPage = 0;
        mutated(store, *doc);
    }

    void removeLayerPressed(EditorStore &store)
    {
        auto *doc = activeTilesetDoc(store);

        if (doc == nullptr)
        {
            return;
        }

        if (doc->sel.layer == 0)
        {
            store.tilesets.message = "the base layer stays";
            return;
        }

        pushTilesetSnapshot(*doc);
        tileset::removeLayer(doc->data, doc->sel.layer);
        clampSelection(*doc);

        if (doc->sel.layer == 0
            && store.tilesets.tool == TilesetTool::Decor)
        {
            store.tilesets.tool = TilesetTool::Draw;
        }

        store.tilesets.libraryPage = 0;
        mutated(store, *doc);
    }

    void addSpritePressed(EditorStore &store)
    {
        auto *doc = activeTilesetDoc(store);

        if (doc == nullptr)
        {
            return;
        }

        clampSelection(*doc);
        pushTilesetSnapshot(*doc);
        static_cast<void>(
            tileset::addSprite(doc->data, doc->sel.layer));
        doc->sel.sprite =
            doc->data.layers[doc->sel.layer].sprites.size() - 1;
        store.tilesets.libraryPage =
            doc->sel.sprite / kLibraryPageSize;
        mutated(store, *doc);
    }

    void duplicateSpritePressed(EditorStore &store)
    {
        auto *doc = activeTilesetDoc(store);

        if (doc == nullptr)
        {
            return;
        }

        const auto *sprite = selectedSprite(*doc);

        if (sprite == nullptr)
        {
            return;
        }

        const auto source = *sprite;

        pushTilesetSnapshot(*doc);

        auto &copy = tileset::addSprite(doc->data, doc->sel.layer);

        copy.frameCount = source.frameCount;
        copy.frames = source.frames;
        copy.sockets = source.sockets;
        copy.on = source.on;
        doc->sel.sprite =
            doc->data.layers[doc->sel.layer].sprites.size() - 1;
        store.tilesets.libraryPage =
            doc->sel.sprite / kLibraryPageSize;
        mutated(store, *doc);
    }

    void deleteSpriteConfirmed(EditorStore &store)
    {
        auto *doc = activeTilesetDoc(store);

        if (doc == nullptr || selectedSprite(*doc) == nullptr)
        {
            return;
        }

        pushTilesetSnapshot(*doc);
        tileset::removeSprite(
            doc->data, doc->sel.layer, doc->sel.sprite);
        clampSelection(*doc);
        mutated(store, *doc);
    }

    void addSocketPressed(EditorStore &store)
    {
        auto *doc = activeTilesetDoc(store);
        auto &tilesets = store.tilesets;

        if (doc == nullptr)
        {
            return;
        }

        const auto &name = tilesets.socketNameField.text;

        if (name.empty())
        {
            tilesets.message = "enter a socket name";
            return;
        }

        if (socketNameTaken(doc->data, name))
        {
            tilesets.message = "name taken";
            return;
        }

        if (doc->data.socketNames.size()
            >= 2 + kMaxNamedSockets)
        {
            tilesets.message = "socket limit reached";
            return;
        }

        pushTilesetSnapshot(*doc);
        tilesets.activeSocket =
            tileset::internSocket(doc->data, name);
        mutated(store, *doc);
    }

    void renameSocketPressed(EditorStore &store)
    {
        auto *doc = activeTilesetDoc(store);
        auto &tilesets = store.tilesets;

        if (doc == nullptr)
        {
            return;
        }

        if (!tilesets.activeSocket.has_value()
            || *tilesets.activeSocket < 2
            || *tilesets.activeSocket
                   >= doc->data.socketNames.size())
        {
            tilesets.message = "pick a named socket";
            return;
        }

        const auto &name = tilesets.socketNameField.text;

        if (name.empty())
        {
            tilesets.message = "enter a socket name";
            return;
        }

        if (socketNameTaken(doc->data, name))
        {
            tilesets.message = "name taken";
            return;
        }

        pushTilesetSnapshot(*doc);
        doc->data.socketNames[*tilesets.activeSocket] = name;
        mutated(store, *doc);
    }

    void deleteSocketPressed(EditorStore &store)
    {
        auto *doc = activeTilesetDoc(store);
        auto &tilesets = store.tilesets;

        if (doc == nullptr)
        {
            return;
        }

        if (!tilesets.activeSocket.has_value()
            || *tilesets.activeSocket < 2
            || *tilesets.activeSocket
                   >= doc->data.socketNames.size())
        {
            tilesets.message = "pick a named socket";
            return;
        }

        const auto socket =
            static_cast<SocketId>(*tilesets.activeSocket);

        if (socketInUse(doc->data, socket))
        {
            tilesets.message = "socket in use";
            return;
        }

        pushTilesetSnapshot(*doc);
        doc->data.socketNames.erase(
            doc->data.socketNames.begin() + socket);

        for (auto &layer : doc->data.layers)
        {
            for (auto &sprite : layer.sprites)
            {
                for (auto &held : sprite.sockets)
                {
                    if (held > socket)
                    {
                        --held;
                    }
                }
            }
        }

        tilesets.activeSocket.reset();
        mutated(store, *doc);
    }

    void setDecorAll(EditorStore &store, const bool allowed)
    {
        auto *doc = activeTilesetDoc(store);

        if (doc == nullptr || !decorMode(store.tilesets, *doc))
        {
            return;
        }

        auto *sprite = selectedSprite(*doc);

        if (sprite == nullptr)
        {
            return;
        }

        std::vector<tileset::SpriteId> wanted{};

        if (allowed)
        {
            for (const auto &base : doc->data.layers[0].sprites)
            {
                wanted.push_back(base.id);
            }
        }

        if (sprite->on == wanted)
        {
            return;
        }

        pushTilesetSnapshot(*doc);
        sprite->on = std::move(wanted);
        mutated(store, *doc);
    }

    void adjustDensity(EditorStore &store, const std::int32_t delta)
    {
        auto *doc = activeTilesetDoc(store);

        if (doc == nullptr || doc->sel.layer < 1)
        {
            return;
        }

        auto &layer = doc->data.layers[doc->sel.layer];
        const auto stepped =
            static_cast<std::int32_t>(layer.density) + delta;
        const auto value =
            static_cast<std::uint8_t>(std::clamp(stepped, 0, 255));

        if (value == layer.density)
        {
            return;
        }

        pushTilesetSnapshot(*doc);
        layer.density = value;
        mutated(store, *doc);
    }

    void adjustWeight(EditorStore &store, const std::int32_t delta)
    {
        auto *doc = activeTilesetDoc(store);

        if (doc == nullptr)
        {
            return;
        }

        auto *sprite = selectedSprite(*doc);

        if (sprite == nullptr)
        {
            return;
        }

        const auto stepped =
            static_cast<std::int32_t>(sprite->weight) + delta;
        const auto low =
            static_cast<std::int32_t>(tileset::kMinWeight);
        const auto high =
            static_cast<std::int32_t>(tileset::kMaxWeight);
        const auto value =
            static_cast<std::uint8_t>(std::clamp(stepped, low, high));

        if (value == sprite->weight)
        {
            return;
        }

        pushTilesetSnapshot(*doc);
        sprite->weight = value;
        mutated(store, *doc);
    }

    void createTilesetPressed(EditorStore &store)
    {
        auto &dialog = store.newTileset;
        auto &tilesets = store.tilesets;
        const auto &name = dialog.nameField.text;

        if (name.empty())
        {
            dialog.message = "enter a name";
            return;
        }

        const auto openTaken = std::ranges::any_of(
            tilesets.open,
            [&name](const TilesetDoc &doc)
            { return doc.data.name == name; });
        const auto listed =
            tileset::listTilesets(tilesets.directory);
        const auto diskTaken =
            std::ranges::find(listed, name) != listed.end();

        if (openTaken || diskTaken)
        {
            dialog.message = "name taken";
            return;
        }

        tilesets.open.emplace_back();

        auto &doc = tilesets.open.back();

        doc.data.name = name;
        doc.data.terrain =
            enums::at<tilemap::TerrainClass>(dialog.terrain);
        static_cast<void>(tileset::addSprite(doc.data, 0));
        doc.path = tilesets.directory / name;
        doc.dirty = true;
        tilesets.active = tilesets.open.size() - 1;
        tilesets.libraryPage = 0;
        tilesets.activeSocket.reset();
        tilesets.tool = TilesetTool::Draw;
        tilesets.message.clear();
        dialog.open = false;
    }

    void saveActiveTileset(
        EditorStore &store, log::ILogger &logger)
    {
        auto *doc = activeTilesetDoc(store);
        auto &tilesets = store.tilesets;

        if (doc == nullptr)
        {
            tilesets.message = "no tileset open";
            return;
        }

        if (doc->path.empty())
        {
            doc->path = tilesets.directory / doc->data.name;
        }

        try
        {
            tileset::saveTileset(doc->path, doc->data);
            doc->dirty = false;
            tilesets.message.clear();
            logger.log(
                log::Level::Info,
                "map_editor: saved " + doc->path.string());
        }
        catch (const tileset::TilesetError &error) // GCOVR_EXCL_LINE
        {
            tilesets.message = error.what();
            logger.log(log::Level::Error, error.what());
        }
    }

    namespace
    {
        void drawOutline(
            gfx::ViewportRenderer &view,
            const RectF &rect,
            const Color color)
        {
            view.drawRect(
                RectF(rect.origin, {rect.size.width, 1.0F}), color);
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

        void drawChecker(
            gfx::ViewportRenderer &view,
            const std::int32_t left,
            const std::int32_t top,
            const std::int32_t extent,
            const std::int32_t step,
            const bool dim)
        {
            for (std::int32_t y = 0; y < extent; y += step)
            {
                for (std::int32_t x = 0; x < extent; x += step)
                {
                    const bool light =
                        ((x / step) + (y / step)) % 2 == 0;
                    const auto color =
                        dim ? (light ? kDimBackdrop : kBackdropDark)
                            : (light ? kBackdropLight
                                     : kBackdropDark);

                    view.drawRect(
                        RectF(
                            {static_cast<float>(left + x),
                             static_cast<float>(top + y)},
                            {static_cast<float>(step),
                             static_cast<float>(step)}),
                        color);
                }
            }
        }

        void drawBandFill(
            gfx::ViewportRenderer &view,
            const BandRect &band,
            const SocketId socket)
        {
            view.drawRect(
                RectF(
                    {static_cast<float>(band.x),
                     static_cast<float>(band.y)},
                    {static_cast<float>(band.width),
                     static_cast<float>(band.height)}),
                socketColor(socket));

            if (socket != kEdgeSocket)
            {
                return;
            }

            const auto hatch = antwika::ui::Theme{}.focusRing;
            const bool horizontal = band.width >= band.height;
            const auto length =
                horizontal ? band.width : band.height;

            for (std::int32_t at = 0; at + 2 <= length; at += 4)
            {
                const auto across = (at / 4) % 2 == 0 ? 1 : 5;
                const auto x =
                    horizontal ? band.x + at : band.x + across;
                const auto y =
                    horizontal ? band.y + across : band.y + at;

                view.drawRect(
                    RectF(
                        {static_cast<float>(x),
                         static_cast<float>(y)},
                        {2.0F, 2.0F}),
                    hatch);
            }
        }

        [[nodiscard]] std::optional<Point> workspaceHover(
            const EditorStore &store)
        {
            const auto &pointer = store.input.canvasPointer;
            const bool underConsole =
                store.input.consoleVisible
                && pointer.has_value()
                && pointer->y < store.input.consoleHeightCanvas;

            if (!pointer.has_value() || store.ui.pointerOverUi
                || store.ui.openMenu.has_value()
                || modalOpen(store) || underConsole
                || pointer->x >= kMapViewWidth
                || pointer->y < kMenuBarHeight)
            {
                return std::nullopt;
            }

            return pointer;
        }

        void drawCaption(
            gfx::ViewportRenderer &view,
            const EditorStore &store,
            const TilesetDoc &doc)
        {
            const auto &data = doc.data;
            const auto layerAt =
                std::min(doc.sel.layer, data.layers.size() - 1);
            const auto &layer = data.layers[layerAt];
            const auto caption =
                data.name + " - "
                + std::string(tilemap::toString(data.terrain))
                + " - L" + std::to_string(layerAt) + " "
                + layer.name + " - f"
                + std::to_string(doc.sel.frame + 1);

            view.drawText(
                {8.0F, 13.0F},
                caption,
                gfx::encodeTextScale(gfx::TextFace::Small, 1),
                kLabelColor);

            const bool decor = decorMode(store.tilesets, doc);
            const auto shown =
                decor ? data.layers[0].sprites.size()
                      : layer.sprites.size();
            const auto pages =
                libraryPageCount(store.tilesets, doc);
            const auto status =
                std::to_string(shown) + " sprites  pg "
                + std::to_string(store.tilesets.libraryPage + 1)
                + "/" + std::to_string(pages);
            const auto width = static_cast<float>(
                status.size() * gfx::kSmallGlyphAdvance);

            view.drawText(
                {316.0F - width, 13.0F},
                status,
                gfx::encodeTextScale(gfx::TextFace::Small, 1),
                antwika::ui::Theme{}.muted);
        }

        void drawEditor(
            gfx::ViewportRenderer &view,
            const EditorStore &store,
            const TilesetDoc &doc,
            const gfx::ITexture *atlas,
            const tileset::AtlasIndex &index)
        {
            const auto left =
                static_cast<float>(kTilesetEditorLeft);
            const auto top = static_cast<float>(kTilesetEditorTop);
            const auto zoom =
                static_cast<float>(kTilesetEditorZoom);

            drawChecker(
                view,
                kTilesetEditorLeft,
                kTilesetEditorTop,
                kEditorExtent,
                kTilesetEditorZoom,
                false);

            const auto *sprite = selectedSprite(doc);
            const auto layerAt =
                std::min(doc.sel.layer, lastLayerIndex(doc));

            if (sprite != nullptr && atlas != nullptr
                && doc.sel.frame < sprite->frameCount
                && layerAt < index.layerRowOffsets.size())
            {
                const auto row =
                    index.layerRowOffsets[layerAt] + doc.sel.sprite;

                view.drawTexture(
                    *atlas,
                    tileset::atlasSource(
                        static_cast<std::uint32_t>(row),
                        static_cast<std::uint8_t>(doc.sel.frame)),
                    RectF(
                        {left, top},
                        {static_cast<float>(kEditorExtent),
                         static_cast<float>(kEditorExtent)}),
                    kWhite);
            }

            for (std::int32_t at = 0; at <= kSpriteSide; ++at)
            {
                const auto lineX =
                    left + static_cast<float>(at) * zoom;
                const auto lineY =
                    top + static_cast<float>(at) * zoom;

                view.drawLine(
                    {lineX, top},
                    {lineX, top + static_cast<float>(kEditorExtent)},
                    kPixelGridColor);
                view.drawLine(
                    {left, lineY},
                    {left + static_cast<float>(kEditorExtent),
                     lineY},
                    kPixelGridColor);
            }

            for (const auto side : enums::kAll<Side>)
            {
                const auto socket =
                    sprite != nullptr
                        ? sprite->sockets[enums::index(side)]
                        : kOpenSocket;

                drawBandFill(view, bandRect(side), socket);
            }

            const auto hover = workspaceHover(store);

            if (!hover.has_value())
            {
                return;
            }

            if (const auto side = socketBandAt(*hover))
            {
                const auto band = bandRect(*side);
                const RectF outline(
                    {static_cast<float>(band.x),
                     static_cast<float>(band.y)},
                    {static_cast<float>(band.width),
                     static_cast<float>(band.height)});

                drawOutline(
                    view, outline, antwika::ui::Theme{}.focusRing);
            }

            if (const auto pixel = editorPixelAt(*hover))
            {
                drawPixelOutline(
                    view,
                    {left + static_cast<float>(pixel->x) * zoom,
                     top + static_cast<float>(pixel->y) * zoom},
                    zoom);
            }
        }

        void drawFrameStrip(
            gfx::ViewportRenderer &view,
            const TilesetDoc &doc,
            const gfx::ITexture *atlas,
            const tileset::AtlasIndex &index,
            const std::uint32_t tick)
        {
            view.drawText(
                {static_cast<float>(kTilesetEditorLeft), 183.0F},
                "frames",
                gfx::encodeTextScale(gfx::TextFace::Small, 1),
                antwika::ui::Theme{}.muted);

            const auto *sprite = selectedSprite(doc);
            const auto layerAt =
                std::min(doc.sel.layer, lastLayerIndex(doc));
            const auto row =
                layerAt < index.layerRowOffsets.size()
                    ? index.layerRowOffsets[layerAt]
                          + doc.sel.sprite
                    : 0;

            for (std::size_t frame = 0;
                 frame < tileset::kMaxFrames;
                 ++frame)
            {
                const auto left =
                    kTilesetEditorLeft
                    + static_cast<std::int32_t>(frame)
                          * kFramePreviewPitch;
                const bool present =
                    sprite != nullptr
                    && frame < sprite->frameCount;
                const RectF slot(
                    {static_cast<float>(left),
                     static_cast<float>(kFrameStripTop)},
                    {static_cast<float>(kFramePreviewSize),
                     static_cast<float>(kFramePreviewSize)});

                drawChecker(
                    view,
                    left,
                    kFrameStripTop,
                    kFramePreviewSize,
                    4,
                    !present);

                if (present && atlas != nullptr)
                {
                    view.drawTexture(
                        *atlas,
                        tileset::atlasSource(
                            static_cast<std::uint32_t>(row),
                            static_cast<std::uint8_t>(frame)),
                        slot,
                        kWhite);
                }
                else
                {
                    const RectF dot(
                        {static_cast<float>(left + 11),
                         static_cast<float>(kFrameStripTop + 11)},
                        {2.0F, 2.0F});

                    view.drawRect(dot, antwika::ui::Theme{}.muted);
                }

                if (frame == doc.sel.frame)
                {
                    drawOutline(
                        view,
                        slot,
                        antwika::ui::Theme{}.focusRing);
                }
            }

            drawChecker(
                view,
                kAnimPreviewLeft,
                kFrameStripTop,
                kFramePreviewSize,
                4,
                false);

            if (sprite != nullptr && atlas != nullptr)
            {
                const auto frame = static_cast<std::uint8_t>(
                    (tick / 8) % sprite->frameCount);

                view.drawTexture(
                    *atlas,
                    tileset::atlasSource(
                        static_cast<std::uint32_t>(row), frame),
                    RectF(
                        {static_cast<float>(kAnimPreviewLeft),
                         static_cast<float>(kFrameStripTop)},
                        {static_cast<float>(kFramePreviewSize),
                         static_cast<float>(kFramePreviewSize)}),
                    kWhite);
            }
        }

        void drawSocketTicks(
            gfx::ViewportRenderer &view,
            const std::int32_t x,
            const std::int32_t y,
            const Sprite &sprite)
        {
            const std::array<RectF, 4> ticks{
                RectF(
                    {static_cast<float>(x + 4),
                     static_cast<float>(y)},
                    {8.0F, 2.0F}),
                RectF(
                    {static_cast<float>(x + kLibraryArt - 2),
                     static_cast<float>(y + 4)},
                    {2.0F, 8.0F}),
                RectF(
                    {static_cast<float>(x + 4),
                     static_cast<float>(y + kLibraryArt - 2)},
                    {8.0F, 2.0F}),
                RectF(
                    {static_cast<float>(x),
                     static_cast<float>(y + 4)},
                    {2.0F, 8.0F})};

            for (const auto side : enums::kAll<Side>)
            {
                view.drawRect(
                    ticks[enums::index(side)],
                    socketColor(
                        sprite.sockets[enums::index(side)]));
            }
        }

        void drawLibrary(
            gfx::ViewportRenderer &view,
            const EditorStore &store,
            const TilesetDoc &doc,
            const gfx::ITexture *atlas,
            const tileset::AtlasIndex &index)
        {
            const auto &tilesets = store.tilesets;
            const bool decor = decorMode(tilesets, doc);
            const auto layerAt =
                decor ? 0
                      : std::min(
                          doc.sel.layer, lastLayerIndex(doc));
            const auto &shown = doc.data.layers[layerAt].sprites;
            const auto *sprite = selectedSprite(doc);
            const auto first =
                tilesets.libraryPage * kLibraryPageSize;
            const auto rowOffset =
                layerAt < index.layerRowOffsets.size()
                    ? index.layerRowOffsets[layerAt]
                    : 0;
            const auto hover = workspaceHover(store);
            const auto hoverCell =
                hover.has_value() ? libraryCellAt(*hover)
                                  : std::nullopt;

            for (std::size_t cell = 0; cell < kLibraryPageSize;
                 ++cell)
            {
                const auto at = first + cell;
                const auto x =
                    kLibraryLeft
                    + static_cast<std::int32_t>(
                          cell % kLibraryColumns)
                          * kLibraryPitch;
                const auto y =
                    kLibraryTop
                    + static_cast<std::int32_t>(
                          cell / kLibraryColumns)
                          * kLibraryPitch;
                const RectF cellRect(
                    {static_cast<float>(x),
                     static_cast<float>(y)},
                    {static_cast<float>(kLibraryArt),
                     static_cast<float>(kLibraryArt)});

                if (at < shown.size())
                {
                    const bool allowed =
                        decor && sprite != nullptr
                        && std::ranges::find(
                               sprite->on, shown[at].id)
                               != sprite->on.end();

                    if (decor && allowed)
                    {
                        view.drawRect(cellRect, kAllowedBackdrop);
                    }
                    else
                    {
                        drawChecker(
                            view, x, y, kLibraryArt, 4, false);
                    }

                    if (atlas != nullptr)
                    {
                        view.drawTexture(
                            *atlas,
                            tileset::atlasSource(
                                static_cast<std::uint32_t>(
                                    rowOffset + at),
                                0),
                            cellRect,
                            kWhite);
                    }

                    if (decor && !allowed)
                    {
                        view.drawRect(cellRect, kDimOverlay);
                    }

                    if (decor && allowed)
                    {
                        view.drawRect(
                            RectF(
                                {static_cast<float>(x),
                                 static_cast<float>(y)},
                                {3.0F, 3.0F}),
                            antwika::ui::Theme{}.focusRing);
                    }

                    if (!decor)
                    {
                        drawSocketTicks(view, x, y, shown[at]);
                    }

                    if (!decor && at == doc.sel.sprite)
                    {
                        drawOutline(
                            view,
                            cellRect,
                            antwika::ui::Theme{}.focusRing);
                    }
                }
                else if (!decor && at == shown.size())
                {
                    drawChecker(view, x, y, kLibraryArt, 4, true);
                    view.drawText(
                        {static_cast<float>(x + 6),
                         static_cast<float>(y + 5)},
                        "+",
                        gfx::encodeTextScale(
                            gfx::TextFace::Small, 1),
                        kLabelColor);
                }
                else
                {
                    continue;
                }

                if (hoverCell.has_value() && *hoverCell == cell)
                {
                    drawOutline(
                        view,
                        cellRect,
                        antwika::ui::Theme{}.focusRing);
                }
            }
        }

        [[nodiscard]] std::uint8_t previewFrame(
            const Sprite &sprite, const std::uint32_t tick) noexcept
        {
            if (sprite.frameCount <= 1)
            {
                return 0;
            }

            return static_cast<std::uint8_t>(
                (tick / kPreviewFramePeriod) % sprite.frameCount);
        }

        void drawPreviewLayer(
            gfx::ViewportRenderer &view,
            const gfx::ITexture &atlas,
            const std::uint32_t rowOffset,
            const std::vector<Sprite> &sprites,
            const std::array<std::int32_t, kPreviewCells> &cells,
            const std::uint32_t tick)
        {
            for (std::size_t at = 0; at < kPreviewCells; ++at)
            {
                if (cells[at] < 0)
                {
                    continue;
                }

                const auto pick =
                    static_cast<std::size_t>(cells[at]);

                if (pick >= sprites.size())
                {
                    continue;
                }

                const auto column = static_cast<std::int32_t>(
                    at % static_cast<std::size_t>(kPreviewColumns));
                const auto row = static_cast<std::int32_t>(
                    at / static_cast<std::size_t>(kPreviewColumns));

                view.drawTexture(
                    atlas,
                    tileset::atlasSource(
                        rowOffset + static_cast<std::uint32_t>(pick),
                        previewFrame(sprites[pick], tick)),
                    RectF(
                        {static_cast<float>(
                             kPreviewLeft + column * kSpriteSide),
                         static_cast<float>(
                             kPreviewTop + row * kSpriteSide)},
                        {static_cast<float>(kSpriteSide),
                         static_cast<float>(kSpriteSide)}),
                    kWhite);
            }
        }

        void drawPreviewControls(
            gfx::ViewportRenderer &view, const EditorStore &store)
        {
            const auto hover = workspaceHover(store);
            const auto theme = antwika::ui::Theme{};
            const RectF regen(
                {static_cast<float>(kRegenRect.x),
                 static_cast<float>(kRegenRect.y)},
                {static_cast<float>(kRegenRect.width),
                 static_cast<float>(kRegenRect.height)});
            const RectF box(
                {static_cast<float>(kAutoRect.x),
                 static_cast<float>(kAutoRect.y)},
                {static_cast<float>(kAutoRect.width),
                 static_cast<float>(kAutoRect.height)});

            view.drawRect(regen, kDimBackdrop);
            drawOutline(
                view,
                regen,
                hover.has_value() && overPreviewRegen(*hover)
                    ? theme.focusRing
                    : theme.muted);
            view.drawText(
                {static_cast<float>(kRegenRect.x + 5),
                 static_cast<float>(kRegenRect.y + 3)},
                "regen",
                gfx::encodeTextScale(gfx::TextFace::Small, 1),
                kLabelColor);

            view.drawRect(box, kDimBackdrop);
            drawOutline(
                view,
                box,
                hover.has_value() && overPreviewAuto(*hover)
                    ? theme.focusRing
                    : theme.muted);

            if (store.tilesets.previewAuto)
            {
                view.drawRect(
                    RectF(
                        {static_cast<float>(kAutoRect.x + 3),
                         static_cast<float>(kAutoRect.y + 3)},
                        {4.0F, 4.0F}),
                    theme.focusRing);
            }

            view.drawText(
                {static_cast<float>(kAutoRect.x + 14),
                 static_cast<float>(kAutoRect.y + 2)},
                "auto",
                gfx::encodeTextScale(gfx::TextFace::Small, 1),
                theme.muted);
        }

        void drawPreviewPanel(
            gfx::ViewportRenderer &view,
            const EditorStore &store,
            const TilesetDoc &doc,
            const gfx::ITexture *atlas,
            const tileset::AtlasIndex &index,
            const std::uint32_t tick,
            const TilesetPreview *preview)
        {
            for (std::int32_t row = 0; row < kPreviewRows; ++row)
            {
                for (std::int32_t column = 0;
                     column < kPreviewColumns;
                     ++column)
                {
                    drawChecker(
                        view,
                        kPreviewLeft + column * kSpriteSide,
                        kPreviewTop + row * kSpriteSide,
                        kSpriteSide,
                        4,
                        false);
                }
            }

            if (preview != nullptr && atlas != nullptr)
            {
                const auto drawable = std::min(
                    index.layerRowOffsets.size(),
                    preview->decor.size() + 1);
                const auto layers =
                    std::min(doc.data.layers.size(), drawable);

                for (std::size_t layer = 0; layer < layers;
                     ++layer)
                {
                    const auto &cells =
                        layer == 0 ? preview->base
                                   : preview->decor[layer - 1];

                    drawPreviewLayer(
                        view,
                        *atlas,
                        index.layerRowOffsets[layer],
                        doc.data.layers[layer].sprites,
                        cells,
                        tick);
                }
            }

            const RectF center(
                {static_cast<float>(
                     kPreviewLeft
                     + kPreviewCenterColumn * kSpriteSide),
                 static_cast<float>(
                     kPreviewTop + kPreviewCenterRow * kSpriteSide)},
                {static_cast<float>(kSpriteSide),
                 static_cast<float>(kSpriteSide)});

            drawOutline(
                view, center, antwika::ui::Theme{}.focusRing);
            drawPreviewControls(view, store);
        }

    }

    void drawTilesetWorkspace(
        gfx::ViewportRenderer &view,
        const EditorStore &store,
        const gfx::ITexture *atlas,
        const tileset::AtlasIndex &index,
        const std::uint32_t tick,
        const TilesetPreview *preview)
    {
        const auto *doc = activeTilesetDoc(store);

        if (doc == nullptr)
        {
            view.drawText(
                {24.0F, 130.0F},
                "no tilesets - File > New Tileset...",
                gfx::encodeTextScale(gfx::TextFace::Small, 1),
                kLabelColor);
            return;
        }

        if (doc->data.layers.empty())
        {
            return;
        }

        drawCaption(view, store, *doc);
        drawEditor(view, store, *doc, atlas, index);
        drawFrameStrip(view, *doc, atlas, index, tick);
        drawLibrary(view, store, *doc, atlas, index);
        drawPreviewPanel(
            view, store, *doc, atlas, index, tick, preview);
    }

}
