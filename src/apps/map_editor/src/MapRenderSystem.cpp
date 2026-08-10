#include "antwika/map_editor/MapRenderSystem.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>

#include <antwika/autotile/DrawPlan.hpp>
#include <antwika/autotile/SheetLayout.hpp>
#include <antwika/autotile/TilePiece.hpp>
#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Glyphs.hpp>
#include <antwika/gfx/RectF.hpp>

#include "antwika/map_editor/CharacterSheets.hpp"
#include "antwika/map_editor/Components.hpp"
#include "antwika/map_editor/OverlayDraw.hpp"
#include "antwika/map_editor/PaletteMath.hpp"
#include "antwika/map_editor/PlaceholderSheets.hpp"
#include "antwika/map_editor/SheetWorkspace.hpp"

namespace antwika::map_editor
{

    namespace
    {
        using antwika::gfx::Color;
        using antwika::gfx::RectF;

        constexpr Color kPaper{.red = 12, .green = 14, .blue = 16};

        constexpr Color kInk{.red = 214, .green = 224, .blue = 216};

        constexpr Color kWhite{.red = 255, .green = 255, .blue = 255};

        constexpr Color kBlack{.red = 0, .green = 0, .blue = 0};
    }

    MapRenderSystem::MapRenderSystem(
        EditorStore &store, gfx::ViewportRenderer &view)
        : store(store), view(view)
    {
        for (const auto terrain :
             enums::kAll<tilemap::TerrainClass>)
        {
            const auto index = enums::index(terrain);
            const auto &doc = store.tiles.docs[index];

            sheets[index] = view.createTexture(
                doc.image.isComplete()
                    ? doc.image
                    : placeholderSheet(terrain, kWhite));
            revisions[index] = doc.revision;
        }
    }

    void MapRenderSystem::refreshSheets()
    {
        for (const auto terrain :
             enums::kAll<tilemap::TerrainClass>)
        {
            const auto index = enums::index(terrain);
            const auto &doc = store.tiles.docs[index];

            if (doc.revision == revisions[index]
                || !doc.image.isComplete())
            {
                continue;
            }

            sheets[index] = view.createTexture(doc.image);
            revisions[index] = doc.revision;
        }
    }

    void MapRenderSystem::update(World &world, antwika::time::Tick tick)
    {
        if (store.input.quit)
        {
            return;
        }

        refreshSheets();

        view.clear(
            store.view == EditorView::Map
                ? colorOf(store.state.map.header().paper)
                : kPaper);
        view.fillSurround(Color{});

        if (store.view == EditorView::Tiles)
        {
            const auto &doc =
                store.tiles.docs[enums::index(store.state.brush)];
            std::optional<gfx::Point> hover{};

            if (store.input.canvasPointer.has_value())
            {
                hover = sheetPixelAt(*store.input.canvasPointer);
            }

            drawSheetWorkspace(
                view,
                *sheets[enums::index(store.state.brush)],
                doc.image,
                hover);
            return;
        }

        if (store.view == EditorView::Characters)
        {
            drawCharacters(tick);
            return;
        }

        drawMap(tick);
        drawFreeMarks();
        drawGhost();

        for (const auto entity : world.view<Marker, CellRef>())
        {
            const auto &at = world.get<CellRef>(entity);

            drawMarker(
                view,
                store.state.map,
                geometry::GridCell{
                    .column = at.column, .row = at.row},
                world.get<Marker>(entity).kind,
                store.camera);
        }

        if (store.ui.selected.has_value()
            && *store.ui.selected
                   < store.state.map.entities().size())
        {
            drawSelection(
                view,
                store.state.map,
                entityCellOf(
                    store.state.map.entities()[*store.ui.selected]),
                store.camera);
        }

        drawValidatorOverlay(view, store.state, store.camera);
        drawHoverOutline();
    }

    void MapRenderSystem::drawHoverOutline()
    {
        const auto &pointer = store.input.canvasPointer;

        if (!pointer.has_value() || store.ui.pointerOverUi
            || modalOpen(store))
        {
            return;
        }

        const bool overMap = pointer->x >= 0
                             && pointer->x < kMapViewWidth
                             && pointer->y >= kMenuBarHeight;
        const bool underConsole =
            store.input.consoleVisible
            && pointer->y < store.input.consoleHeightCanvas;

        if (!overMap || underConsole)
        {
            return;
        }

        drawHover(
            view, store.state.map, store.state.hovered, store.camera);
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

            slot.texture =
                view.createTexture(character.sheet.image);
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
                kInk);
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
            characters.list[characters.selected].sheet.image,
            hover,
            static_cast<std::uint32_t>(tick));
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
        const auto plan = autotile::buildDrawPlan(
            state.map,
            state.hovered,
            state.map.at(state.hovered).height,
            static_cast<std::uint32_t>(tick));

        const auto zoom = store.camera.zoom();
        const auto ink = colorOf(state.map.header().ink);

        drawBackdrop();

        for (const auto &draw : plan)
        {
            const auto source = autotile::sheetSource(
                draw.piece, draw.mask, draw.variant);
            const auto tint =
                draw.piece == autotile::TilePiece::Shade ? kBlack
                                                         : ink;

            view.drawTexture(
                *sheets[enums::index(draw.terrain)],
                source,
                RectF(
                    {static_cast<float>(draw.screen.x) * zoom
                         + store.camera.panX,
                     static_cast<float>(draw.screen.y) * zoom
                         + store.camera.panY
                         + static_cast<float>(kMenuBarHeight)},
                    {8.0F * zoom, 8.0F * zoom}),
                tint);
        }
    }

}
