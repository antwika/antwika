#include "antwika/map_editor/MapRenderSystem.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <antwika/autotile/DrawPlan.hpp>
#include <antwika/autotile/MissingArt.hpp>
#include <antwika/autotile/SystemSheet.hpp>
#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/GfxError.hpp>
#include <antwika/gfx/Glyphs.hpp>
#include <antwika/gfx/PngReader.hpp>
#include <antwika/gfx/RectF.hpp>

#include "antwika/map_editor/CharacterSheets.hpp"
#include "antwika/map_editor/Components.hpp"
#include "antwika/map_editor/OverlayDraw.hpp"
#include "antwika/map_editor/PaletteMath.hpp"
#include "antwika/map_editor/PlaceholderTilesets.hpp"
#include "antwika/map_editor/Selection.hpp"
#include "antwika/map_editor/SheetWorkspace.hpp"
#include "antwika/map_editor/TilesetWorkspace.hpp"

namespace antwika::map_editor
{

    namespace
    {
        using antwika::gfx::Color;
        using antwika::gfx::RectF;

        constexpr Color kPaper{.red = 12, .green = 14, .blue = 16};

        constexpr Color kWhite{.red = 255, .green = 255, .blue = 255};

        constexpr Color kBlack{.red = 0, .green = 0, .blue = 0};

        constexpr Color kMissing{.red = 255, .green = 0, .blue = 0};

        constexpr std::uint32_t kClockBucket = 30;

        constexpr std::uint64_t kPickPreviewTicks = 180;

        constexpr std::int32_t kPickArtScale = 2;

        constexpr std::int32_t kPickBoxPad = 2;

        constexpr std::int32_t kPickBoxBottom = 258;

        constexpr std::uint32_t kPreviewAutoPeriod = 90;

        constexpr std::uint32_t kSystemWidth = 32;

        constexpr std::uint32_t kSystemHeight = 8;

        [[nodiscard]] gfx::Bitmap loadSystemArt(
            const std::filesystem::path &directory)
        {
            const auto path = directory / "system.png";

            if (std::filesystem::is_regular_file(path))
            {
                try
                {
                    std::ifstream in(path, std::ios::binary);
                    const gfx::PngReader reader;
                    auto bitmap = reader.read(in); // GCOVR_EXCL_LINE

                    if (bitmap.size.width == kSystemWidth
                        && bitmap.size.height == kSystemHeight)
                    {
                        normalizeSheetClasses(bitmap);
                        return bitmap;
                    }
                }
                catch (const gfx::GfxError &) // GCOVR_EXCL_LINE
                {
                }
            }

            return placeholderSystemSheet();
        }

        [[nodiscard]] gfx::Bitmap emptyAtlas()
        {
            gfx::Bitmap bitmap{};

            bitmap.size.width =
                static_cast<std::uint32_t>(tileset::kAtlasWidth);
            bitmap.size.height =
                static_cast<std::uint32_t>(tileset::kSpriteSide);
            bitmap.pixels.assign(
                static_cast<std::size_t>(tileset::kAtlasWidth)
                    * tileset::kSpriteSide * gfx::kBytesPerPixel,
                0);

            return bitmap;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] const TilesetDoc *findOpenDoc(
            const EditorStore &store, const std::string &name)
        {
            for (const auto &doc : store.tilesets.open)
            {
                if (doc.data.name == name)
                {
                    return &doc;
                }
            }

            return nullptr;
        }

        [[nodiscard]] std::optional<std::size_t> openDocIndexOf(
            const EditorStore &store, const std::string &name)
        {
            for (std::size_t at = 0;
                 at < store.tilesets.open.size();
                 ++at)
            {
                if (store.tilesets.open[at].data.name == name)
                {
                    return at;
                }
            }

            return std::nullopt;
        }

        [[nodiscard]] std::int32_t latticeIndexOf(
            const std::int32_t value) noexcept
        {
            const auto side = tileset::kSpriteSide;

            return value >= 0 ? value / side
                              : (value - side + 1) / side;
        }

        [[nodiscard]] gfx::Point latticeCellOf(
            const gfx::Point point) noexcept
        {
            return gfx::Point{
                .x = latticeIndexOf(point.x),
                .y = latticeIndexOf(point.y)};
        }

        [[nodiscard]] std::vector<const autotile::TileDraw *>
        spriteDrawsAt(
            const autotile::DrawPlan &plan, const gfx::Point point)
        {
            std::vector<const autotile::TileDraw *> stack;

            for (const auto &draw : plan)
            {
                if (draw.kind != autotile::DrawKind::Sprite)
                {
                    continue;
                }

                const bool inside =
                    point.x >= draw.screen.x
                    && point.x
                           < draw.screen.x + tileset::kSpriteSide
                    && point.y >= draw.screen.y
                    && point.y
                           < draw.screen.y + tileset::kSpriteSide;

                if (inside)
                {
                    stack.push_back(&draw);
                }
            }

            return stack;
        } // GCOVR_EXCL_LINE

        struct SpriteRef final
        {
            std::size_t layer = 0;
            std::size_t sprite = 0;
        };

        [[nodiscard]] std::optional<SpriteRef> spriteOfAtlasRow(
            const tileset::AtlasIndex &index,
            const std::uint16_t atlasRow)
        {
            if (atlasRow >= index.rows)
            {
                return std::nullopt;
            }

            std::size_t layer = 0;

            for (std::size_t at = 0;
                 at < index.layerRowOffsets.size();
                 ++at)
            {
                if (index.layerRowOffsets[at] <= atlasRow)
                {
                    layer = at;
                }
            }

            return SpriteRef{
                .layer = layer,
                .sprite =
                    atlasRow - index.layerRowOffsets[layer]};
        }

        [[nodiscard]] std::size_t pickDepthOf(
            const PickerState &picker,
            const gfx::Point cell,
            const std::size_t stackSize)
        {
            const bool sameCell = picker.walkCell.has_value()
                                  && *picker.walkCell == cell;

            return sameCell
                       ? (picker.walkDepth + 1) % stackSize
                       : 0;
        }
    }

    MapRenderSystem::MapRenderSystem(
        EditorStore &store, gfx::ViewportRenderer &view)
        : store(store), view(view) // GCOVR_EXCL_LINE
    {
        bakedInk = store.state.map.header().ink;
        bakedPaper = store.state.map.header().paper;

        for (const auto terrain :
             enums::kAll<tilemap::TerrainClass>)
        {
            placeholders[enums::index(terrain)] =
                placeholderTileset(terrain);
        }

        systemArt = loadSystemArt(store.tilesets.directory);
        rebakeStatic();
    }

    void MapRenderSystem::bakeInto(
        BakedTileset &slot,
        const tileset::Tileset &set,
        const std::uint64_t revision)
    {
        slot.name = set.name;
        slot.revision = revision;
        slot.index = tileset::atlasIndexOf(set);
        slot.texture = view.createTexture(
            slot.index.rows == 0
                ? emptyAtlas()
                : tileset::bakeAtlas(
                    set, colorOf(bakedInk), colorOf(bakedPaper)));
    }

    void MapRenderSystem::rebakeStatic()
    {
        for (std::size_t at = 0; at < kTerrainCount; ++at)
        {
            bakeInto(placeholderBaked[at], placeholders[at], 0);
        }

        systemTexture = view.createTexture(bakedSheet(
            systemArt, colorOf(bakedInk), colorOf(bakedPaper)));
    }

    MapRenderSystem::BakedTileset *MapRenderSystem::findBaked(
        const std::string &name)
    {
        for (auto &slot : baked)
        {
            if (slot.name == name)
            {
                return &slot;
            }
        }

        return nullptr;
    }

    void MapRenderSystem::refreshTilesets()
    {
        const auto &header = store.state.map.header();
        const bool rebakeAll =
            header.ink != bakedInk || header.paper != bakedPaper;

        if (rebakeAll)
        {
            bakedInk = header.ink;
            bakedPaper = header.paper;
            baked.clear();
            characterTextures.clear();
            rebakeStatic();
        }

        for (const auto &doc : store.tilesets.open)
        {
            auto *slot = findBaked(doc.data.name);

            if (slot != nullptr && slot->revision == doc.revision)
            {
                continue;
            }

            if (slot == nullptr)
            {
                slot = &baked.emplace_back();
            }

            bakeInto(*slot, doc.data, doc.revision);
        }

        std::erase_if(
            baked,
            [this](const BakedTileset &slot)
            {
                return findOpenDoc(store, slot.name) == nullptr;
            });
    }

    std::array<MapRenderSystem::ResolvedTileset, kTerrainCount>
    MapRenderSystem::resolveBindings()
    {
        const auto &header = store.state.map.header();
        std::array<ResolvedTileset, kTerrainCount> resolved;

        for (const auto terrain :
             enums::kAll<tilemap::TerrainClass>)
        {
            const auto at = enums::index(terrain);
            const auto &bound = header.tilesets[at];
            const TilesetDoc *doc =
                bound.empty() ? nullptr
                              : findOpenDoc(store, bound);

            if (doc == nullptr)
            {
                doc = findOpenDoc(
                    store,
                    "default-"
                        + std::string(tilemap::toString(terrain)));
            }

            if (doc != nullptr)
            {
                const auto *slot = findBaked(doc->data.name);

                resolved[at].set = &doc->data;
                resolved[at].texture = slot->texture.get();
                resolved[at].index = &slot->index;
                resolved[at].name = slot->name;
                resolved[at].revision = doc->revision;
                continue;
            }

            resolved[at].set = &placeholders[at];
            resolved[at].texture = placeholderBaked[at].texture.get();
            resolved[at].index = &placeholderBaked[at].index;
            resolved[at].name = placeholderBaked[at].name;
            resolved[at].revision = 0;
        }

        return resolved;
    } // GCOVR_EXCL_LINE

    void MapRenderSystem::update(World &world, antwika::time::Tick tick)
    {
        if (store.input.quit)
        {
            return;
        }

        refreshTilesets();

        view.clear(
            store.view == EditorView::Map
                ? colorOf(store.state.map.header().paper)
                : kPaper);
        view.fillSurround(Color{});

        if (store.view == EditorView::Tiles)
        {
            drawTiles(tick);
            return;
        }

        if (store.view == EditorView::Characters)
        {
            drawCharacters(tick);
            return;
        }

        drawMap(tick);
        drawActiveLevelMarks(view, store.state, store.camera);
        drawFreeMarks();
        drawGhost();

        for (const auto entity : world.view<Marker, CellRef>())
        {
            const auto &at = world.get<CellRef>(entity);

            drawMarker(
                view,
                geometry::GridCell{
                    .column = at.column, .row = at.row},
                at.level,
                world.get<Marker>(entity).kind,
                store.camera);
        }

        if (store.ui.selected.has_value()
            && *store.ui.selected
                   < store.state.map.entities().size())
        {
            const auto &selected =
                store.state.map.entities()[*store.ui.selected];

            drawSelection(
                view,
                entityCellOf(selected),
                entityLevelOf(selected),
                store.camera);
        }

        drawValidatorOverlay(view, store.state, store.camera);
        drawHoverOutline();
        drawMapSelectionOverlay(view, store);
        drawPickerPreview(tick);
    }

    void MapRenderSystem::updatePicker(
        const std::array<ResolvedTileset, kTerrainCount> &resolved,
        const antwika::time::Tick tick)
    {
        auto &picker = store.picker;

        if (picker.pending.has_value())
        {
            const auto point = *picker.pending;

            picker.pending.reset();
            applyPick(resolved, point, tick);
        }

        picker.hover.clear();

        if (!picker.active)
        {
            return;
        }

        const auto pointer = mapViewPointer();

        if (!pointer.has_value())
        {
            return;
        }

        picker.hover = pickHoverText(
            resolved, mapPointOf(*pointer, store.camera));
    }

    void MapRenderSystem::applyPick(
        const std::array<ResolvedTileset, kTerrainCount> &resolved,
        const gfx::Point point,
        const antwika::time::Tick tick)
    {
        auto &picker = store.picker;
        const auto stack = spriteDrawsAt(cachedPlan, point);

        if (stack.empty())
        {
            return;
        }

        const auto cell = latticeCellOf(point);
        const auto depth =
            pickDepthOf(picker, cell, stack.size());

        picker.walkCell = cell;
        picker.walkDepth = depth;

        const auto &draw = *stack[depth];
        const auto at = enums::index(draw.terrain);
        const auto ref =
            spriteOfAtlasRow(*resolved[at].index, draw.atlasRow);

        if (!ref.has_value())
        {
            return;
        }

        auto label = resolved[at].name + " L"
                     + std::to_string(ref->layer) + " sprite "
                     + std::to_string(ref->sprite);
        const auto doc = openDocIndexOf(store, resolved[at].name);

        if (doc.has_value())
        {
            activateTileset(store, *doc);

            store.tilesets.open[*doc].sel = TilesetSelection{
                .layer = ref->layer,
                .sprite = ref->sprite,
                .frame = 0};

            if (ref->layer == 0
                && store.tilesets.tool == TilesetTool::Decor)
            {
                store.tilesets.tool = TilesetTool::Draw;
            }
        }
        else
        {
            store.tilesets.message =
                "placeholder tileset - not editable";
            label += " - not editable";
        }

        picker.picked.emplace();
        picker.picked->terrain = draw.terrain;
        picker.picked->atlasRow = draw.atlasRow;
        picker.picked->label = std::move(label);
        picker.picked->tick = static_cast<std::uint64_t>(tick);
    }

    std::string MapRenderSystem::pickHoverText(
        const std::array<ResolvedTileset, kTerrainCount> &resolved,
        const gfx::Point point) const
    {
        const auto stack = spriteDrawsAt(cachedPlan, point);

        if (stack.empty())
        {
            return "pick: nothing here";
        }

        const auto depth = pickDepthOf(
            store.picker, latticeCellOf(point), stack.size());
        const auto &draw = *stack[depth];
        const auto at = enums::index(draw.terrain);
        const auto &slot = resolved[at];
        const auto ref =
            spriteOfAtlasRow(*slot.index, draw.atlasRow);

        if (!ref.has_value())
        {
            return "pick: nothing here";
        }

        auto text = "pick: " + slot.name + " L"
                    + std::to_string(ref->layer);

        if (ref->layer < slot.set->layers.size())
        {
            text += " " + slot.set->layers[ref->layer].name;
        }

        text += " (sprite " + std::to_string(ref->sprite) + ")";

        if (findOpenDoc(store, slot.name) == nullptr)
        {
            return text + " - not editable";
        }

        if (stack.size() > 1)
        {
            const auto &next =
                *stack[(depth + 1) % stack.size()];
            const auto nextRef = spriteOfAtlasRow(
                *resolved[enums::index(next.terrain)].index,
                next.atlasRow);

            if (nextRef.has_value())
            {
                text += " - click again for L"
                        + std::to_string(nextRef->layer);
            }
        }

        return text;
    }

    void MapRenderSystem::drawPickerPreview(
        const antwika::time::Tick tick)
    {
        const auto &picker = store.picker;

        if (!picker.picked.has_value())
        {
            return;
        }

        const auto age = static_cast<std::uint64_t>(tick)
                         - picker.picked->tick;

        if (!picker.active && age >= kPickPreviewTicks)
        {
            return;
        }

        const auto resolved = resolveBindings();
        const auto &slot =
            resolved[enums::index(picker.picked->terrain)];

        if (picker.picked->atlasRow >= slot.index->rows)
        {
            return;
        }

        const auto art =
            tileset::kSpriteSide * kPickArtScale;
        const auto labelWidth = static_cast<std::int32_t>(
            picker.picked->label.size()
            * gfx::kSmallGlyphAdvance);
        const auto width = kPickBoxPad + art + kPickBoxPad
                           + labelWidth + kPickBoxPad;
        const auto height = art + 2 * kPickBoxPad;
        const auto left =
            kMapViewWidth - kPickBoxPad - width;
        const auto top = kPickBoxBottom - height;

        view.drawRect(
            RectF(
                {static_cast<float>(left),
                 static_cast<float>(top)},
                {static_cast<float>(width),
                 static_cast<float>(height)}),
            Color{.alpha = 200});
        const auto source =
            tileset::atlasSource(picker.picked->atlasRow, 0);
        const RectF where(
            {static_cast<float>(left + kPickBoxPad),
             static_cast<float>(top + kPickBoxPad)},
            {static_cast<float>(art), static_cast<float>(art)});

        view.drawTexture(*slot.texture, source, where, kWhite);
        view.drawText(
            {static_cast<float>(
                 left + kPickBoxPad + art + kPickBoxPad),
             static_cast<float>(
                 top
                 + (height
                    - static_cast<std::int32_t>(
                        gfx::kSmallGlyphLineHeight))
                       / 2)},
            picker.picked->label,
            gfx::encodeTextScale(gfx::TextFace::Small, 1),
            Color{.red = 214, .green = 224, .blue = 216});
    }

    void MapRenderSystem::drawTiles(const antwika::time::Tick tick)
    {
        const auto clock = static_cast<std::uint32_t>(tick);

        if (store.tilesets.previewAuto
            && clock % kPreviewAutoPeriod == 0)
        {
            ++store.tilesets.previewSeed;
        }

        const auto *doc = activeTilesetDoc(store);
        const auto *slot =
            doc != nullptr ? findBaked(doc->data.name) : nullptr;
        const tileset::AtlasIndex none{};

        drawTilesetWorkspace(
            view,
            store,
            slot != nullptr ? slot->texture.get() : nullptr,
            slot != nullptr ? slot->index : none,
            clock,
            refreshPreview(doc));
        drawTilesSelectionOverlay(view, store);
    }

    const TilesetPreview *MapRenderSystem::refreshPreview(
        const TilesetDoc *doc)
    {
        if (doc == nullptr)
        {
            return nullptr;
        }

        const auto layer = std::min(
            doc->sel.layer, doc->data.layers.size() - 1);
        const auto &sprites = doc->data.layers[layer].sprites;

        if (sprites.empty())
        {
            return nullptr;
        }

        const auto sprite =
            std::min(doc->sel.sprite, sprites.size() - 1);
        PreviewKey key;

        key.name = doc->data.name;
        key.revision = doc->revision;
        key.layer = layer;
        key.sprite = sprite;
        key.seed = store.tilesets.previewSeed;

        if (!previewKey.has_value() || !(*previewKey == key))
        {
            cachedPreview = buildTilesetPreview(
                doc->data,
                layer,
                sprite,
                store.tilesets.previewSeed);
            previewKey = std::move(key);

            if (cachedPreview.centerBaseMissing)
            {
                store.tilesets.message =
                    "no base sprites allowed yet";
            }
        }

        return &cachedPreview;
    }

    std::optional<gfx::Point> MapRenderSystem::mapViewPointer()
        const
    {
        const auto &pointer = store.input.canvasPointer;

        if (!pointer.has_value() || store.ui.pointerOverUi
            || modalOpen(store))
        {
            return std::nullopt;
        }

        const bool overMap = pointer->x >= 0
                             && pointer->x < kMapViewWidth
                             && pointer->y >= kMenuBarHeight;
        const bool underConsole =
            store.input.consoleVisible
            && pointer->y < store.input.consoleHeightCanvas;

        if (!overMap || underConsole)
        {
            return std::nullopt;
        }

        return pointer;
    }

    void MapRenderSystem::drawHoverOutline()
    {
        if (!mapViewPointer().has_value())
        {
            return;
        }

        drawHover(
            view,
            store.state.hovered,
            store.state.activeLevel,
            store.camera);
    }

    void MapRenderSystem::refreshCharacters()
    {
        const auto &list = store.characters.list;

        characterTextures.resize(list.size());

        for (std::size_t index = 0; index < list.size(); ++index)
        {
            auto &slot = characterTextures[index];
            const auto &character = list[index];

            if (slot.texture != nullptr
                && slot.name == character.name
                && slot.revision == character.sheet.revision)
            {
                continue;
            }

            slot.texture = view.createTexture(bakedSheet(
                character.sheet.image,
                colorOf(bakedInk),
                colorOf(bakedPaper)));
            slot.name = character.name;
            slot.revision = character.sheet.revision;
        }
    }

    void MapRenderSystem::drawCharacters(const antwika::time::Tick tick)
    {
        refreshCharacters();

        const auto &characters = store.characters;

        if (characters.selected >= characters.list.size())
        {
            view.drawText(
                {40.0F, 130.0F},
                "no characters - use New in the panel",
                gfx::encodeTextScale(gfx::TextFace::Small, 1),
                Color{.red = 214, .green = 224, .blue = 216});
            return;
        }

        std::optional<gfx::Point> hover{};

        if (store.input.canvasPointer.has_value())
        {
            hover = characterPixelAt(*store.input.canvasPointer);
        }

        drawCharacterWorkspace(
            view,
            *characterTextures[characters.selected].texture,
            hover,
            static_cast<std::uint32_t>(tick));
        drawCharSelectionOverlay(view, store);
    }

    void MapRenderSystem::drawFreeMarks()
    {
        const auto &state = store.state;
        const auto columns = state.map.columns();

        for (std::uint32_t row = 0; row < state.map.rows(); ++row)
        {
            for (std::uint32_t column = 0; column < columns;
                 ++column)
            {
                const auto index =
                    static_cast<std::size_t>(row) * columns + column;

                if (index >= state.pinned.size()
                    || state.pinned[index])
                {
                    continue;
                }

                drawFreeMark(
                    view,
                    state.map,
                    geometry::GridCell{.column = column, .row = row},
                    state.activeLevel,
                    store.camera);
            }
        }
    }

    void MapRenderSystem::drawBackdrop()
    {
        const auto zoom = store.camera.zoom();
        const auto step = 16.0F * zoom;
        const auto chrome =
            chromeFor(store.state.map.header().paper);

        view.drawRect(
            RectF(
                {0.0F, static_cast<float>(kMenuBarHeight)},
                {static_cast<float>(kMapViewWidth),
                 static_cast<float>(kMapViewHeight)}),
            chrome.voidColor);

        for (std::uint32_t row = 0; row < store.state.map.rows();
             ++row)
        {
            for (std::uint32_t column = 0;
                 column < store.state.map.columns();
                 ++column)
            {
                view.drawRect(
                    RectF(
                        {static_cast<float>(column) * step
                             + store.camera.panX,
                         static_cast<float>(row) * step
                             + store.camera.panY
                             + static_cast<float>(kMenuBarHeight)},
                        {step, step}),
                    (column + row) % 2 == 0 ? chrome.checkerLight
                                            : chrome.checkerDark);
            }
        }
    }

    void MapRenderSystem::drawGhost()
    {
        if (!store.state.hoveredBeyond.has_value())
        {
            return;
        }

        const auto zoom = store.camera.zoom();
        const auto step = 16.0F * zoom;
        const auto &cell = *store.state.hoveredBeyond;
        const auto chrome =
            chromeFor(store.state.map.header().paper);
        const auto left = static_cast<float>(cell.column) * step
                          + store.camera.panX;
        const auto top = static_cast<float>(cell.row) * step
                         + store.camera.panY
                         + static_cast<float>(kMenuBarHeight);

        view.drawRect(
            RectF({left, top}, {step, step}), chrome.ghostFill);
        view.drawLine(
            {left, top}, {left + step, top}, chrome.ghostEdge);
        view.drawLine(
            {left + step, top},
            {left + step, top + step},
            chrome.ghostEdge);
        view.drawLine(
            {left + step, top + step},
            {left, top + step},
            chrome.ghostEdge);
        view.drawLine(
            {left, top + step}, {left, top}, chrome.ghostEdge);
    }

    void MapRenderSystem::drawMap(const antwika::time::Tick tick)
    {
        const auto &state = store.state;
        const auto resolved = resolveBindings();
        const auto clock = static_cast<std::uint32_t>(tick);

        PlanKey key{
            .mapRevision = state.revision,
            .player = state.hovered,
            .playerHeight = state.activeLevel,
            .clockBucket = clock / kClockBucket};

        for (std::size_t at = 0; at < kTerrainCount; ++at)
        {
            key.names[at] = resolved[at].name;
            key.revisions[at] = resolved[at].revision;
        }

        if (!planKey.has_value() || !(*planKey == key))
        {
            autotile::TilesetBindings bindings{};

            for (std::size_t at = 0; at < kTerrainCount; ++at)
            {
                bindings.byTerrain[at] = resolved[at].set;
            }

            cachedPlan = autotile::buildDrawPlan(
                state.map,
                state.hovered,
                state.activeLevel,
                clock,
                bindings);
            planKey = key;
        }

        updatePicker(resolved, tick);

        const auto zoom = store.camera.zoom();

        drawBackdrop();

        for (const auto &draw : cachedPlan)
        {
            const auto &slot = resolved[enums::index(draw.terrain)];
            const bool sprite =
                draw.kind == autotile::DrawKind::Sprite;
            const auto source =
                sprite ? tileset::atlasSource(
                    draw.atlasRow, draw.frame)
                       : autotile::systemSource(draw.kind);
            const RectF target(
                {static_cast<float>(draw.screen.x) * zoom
                     + store.camera.panX,
                 static_cast<float>(draw.screen.y) * zoom
                     + store.camera.panY
                     + static_cast<float>(kMenuBarHeight)},
                {static_cast<float>(source.size.width) * zoom,
                 static_cast<float>(source.size.height) * zoom});

            if (autotile::artMissing(draw, *slot.set, *slot.index))
            {
                view.drawRect(target, kMissing);
                continue;
            }

            view.drawTexture(
                sprite ? *slot.texture : *systemTexture,
                source,
                target,
                draw.kind == autotile::DrawKind::Shade ? kBlack
                                                       : kWhite);
        }
    }

}
