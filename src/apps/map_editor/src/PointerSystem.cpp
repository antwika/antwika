#include "antwika/map_editor/PointerSystem.hpp"

#include <cmath>
#include <cstdint>
#include <optional>
#include <variant>

#include <antwika/gfx/Point.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/MouseButton.hpp>

#include "antwika/map_editor/CharacterSheets.hpp"
#include "antwika/map_editor/Commands.hpp"
#include "antwika/map_editor/SheetWorkspace.hpp"

namespace antwika::map_editor
{

    namespace
    {
        using antwika::gfx::Point;
        using antwika::input::MouseButton;
        using antwika::input::PointerButtonPressed;
        using antwika::input::PointerButtonReleased;
        using antwika::input::PointerMoved;
        using antwika::input::PointerScrolled;

        constexpr float kWheelPan = 16.0F;

        [[nodiscard]] bool consoleCovers(
            const EditorStore &store, const Point canvas) noexcept
        {
            return store.input.consoleVisible
                   && canvas.y < store.input.consoleHeightCanvas;
        }

        [[nodiscard]] bool overMap(const Point canvas) noexcept
        {
            return canvas.x >= 0 && canvas.x < kMapViewWidth
                   && canvas.y >= kMenuBarHeight;
        }

        [[nodiscard]] Point mapPoint(
            const Point canvas, const MapCamera &camera) noexcept
        {
            const auto zoom = camera.zoom();
            const auto localX =
                static_cast<float>(canvas.x) - camera.panX;
            const auto localY =
                static_cast<float>(canvas.y - kMenuBarHeight)
                - camera.panY;

            return Point{
                .x = static_cast<std::int32_t>(localX / zoom),
                .y = static_cast<std::int32_t>(localY / zoom)};
        }

        [[nodiscard]] SignedCell signedCellUnder(
            const Point canvas, const MapCamera &camera) noexcept
        {
            const auto zoom = camera.zoom();
            const auto mapX =
                (static_cast<float>(canvas.x) - camera.panX) / zoom;
            const auto mapY =
                (static_cast<float>(canvas.y - kMenuBarHeight)
                 - camera.panY)
                / zoom;

            return SignedCell{
                .column = static_cast<std::int32_t>(
                    std::floor(mapX / 16.0F)),
                .row = static_cast<std::int32_t>(
                    std::floor(mapY / 16.0F))};
        }

        [[nodiscard]] bool insideMap(
            const EditorStore &store, const SignedCell cell) noexcept
        {
            return cell.column >= 0 && cell.row >= 0
                   && cell.column < static_cast<std::int32_t>(
                          store.state.map.columns())
                   && cell.row < static_cast<std::int32_t>(
                          store.state.map.rows());
        }

        [[nodiscard]] std::optional<SignedCell> beyondEdge(
            const EditorStore &store, const SignedCell cell) noexcept
        {
            if (insideMap(store, cell))
            {
                return std::nullopt;
            }

            const auto columns =
                static_cast<std::int32_t>(store.state.map.columns());
            const auto rows =
                static_cast<std::int32_t>(store.state.map.rows());

            if (cell.column < -kExtendMargin
                || cell.row < -kExtendMargin
                || cell.column >= columns + kExtendMargin
                || cell.row >= rows + kExtendMargin)
            {
                return std::nullopt;
            }

            return cell;
        }

        [[nodiscard]] float mapPixelWidth(const EditorStore &store)
        {
            return static_cast<float>(store.state.map.columns())
                   * 16.0F;
        }

        [[nodiscard]] float mapPixelHeight(const EditorStore &store)
        {
            return static_cast<float>(store.state.map.rows()) * 16.0F;
        }
    }

    PointerSystem::PointerSystem(
        EditorStore &store, const gfx::ViewportRenderer &view)
        : store(store), view(view)
    {
    }

    void PointerSystem::update(World &, antwika::time::Tick)
    {
        auto &input = store.input;
        const bool tilesView = store.view != EditorView::Map;
        const auto workspacePixel = [this](const Point canvas)
        {
            return store.view == EditorView::Tiles
                       ? sheetPixelAt(canvas)
                       : characterPixelAt(canvas);
        };

        input.pressed = false;
        input.gestures.clear();
        input.sheetGestures.clear();

        for (const auto &event : input.events)
        {
            if (const auto *moved = std::get_if<PointerMoved>(&event))
            {
                const auto canvas = view.viewport().toCanvas(Point{
                    .x = moved->position.x, .y = moved->position.y});

                input.canvasPointer = canvas;

                if (tilesView)
                {
                    if (const auto pixel = workspacePixel(canvas))
                    {
                        input.sheetGestures.push_back(SheetGesture{
                            .kind = GestureKind::Move,
                            .pixel = *pixel});
                    }

                    continue;
                }

                if (input.panning)
                {
                    store.camera.panX += static_cast<float>(
                        canvas.x - input.panAnchor.x);
                    store.camera.panY += static_cast<float>(
                        canvas.y - input.panAnchor.y);
                    input.panAnchor = canvas;
                    clampCamera(
                        store.camera,
                        mapPixelWidth(store),
                        mapPixelHeight(store));
                }

                if (overMap(canvas))
                {
                    const auto signedCell =
                        signedCellUnder(canvas, store.camera);

                    store.state.hoveredBeyond =
                        beyondEdge(store, signedCell);

                    if (insideMap(store, signedCell))
                    {
                        store.state.hovered = cellUnder(
                            store.state.map,
                            mapPoint(canvas, store.camera));
                    }

                    input.gestures.push_back(MapGesture{
                        .kind = GestureKind::Move,
                        .cell = cellUnder(
                            store.state.map,
                            mapPoint(canvas, store.camera)),
                        .signedCell = signedCell});
                }
                else
                {
                    store.state.hoveredBeyond.reset();
                }

                continue;
            }

            if (const auto *scrolled =
                    std::get_if<PointerScrolled>(&event))
            {
                if (tilesView || modalOpen(store)
                    || !input.canvasPointer.has_value()
                    || consoleCovers(store, *input.canvasPointer)
                    || !overMap(*input.canvasPointer))
                {
                    continue;
                }

                if (scrolled->vertical != 0)
                {
                    zoomAt(
                        store.camera,
                        static_cast<float>(input.canvasPointer->x),
                        static_cast<float>(
                            input.canvasPointer->y - kMenuBarHeight),
                        scrolled->vertical,
                        mapPixelWidth(store),
                        mapPixelHeight(store));
                }

                if (scrolled->horizontal != 0)
                {
                    store.camera.panX -=
                        static_cast<float>(scrolled->horizontal)
                        * kWheelPan;
                    clampCamera(
                        store.camera,
                        mapPixelWidth(store),
                        mapPixelHeight(store));
                }

                continue;
            }

            if (const auto *down =
                    std::get_if<PointerButtonPressed>(&event))
            {
                const auto canvas = view.viewport().toCanvas(Point{
                    .x = down->position.x, .y = down->position.y});

                input.canvasPointer = canvas;

                if (consoleCovers(store, canvas))
                {
                    continue;
                }

                if (down->button == MouseButton::Middle)
                {
                    if (!tilesView && overMap(canvas)
                        && !modalOpen(store))
                    {
                        input.panning = true;
                        input.panAnchor = canvas;
                    }

                    continue;
                }

                if (tilesView
                    && (down->button == MouseButton::Left
                        || down->button == MouseButton::Right))
                {
                    if (down->button == MouseButton::Left)
                    {
                        input.down = true;
                        input.pressed = true;
                    }

                    if (!store.ui.pointerOverUi
                        && !store.ui.openMenu.has_value()
                        && !modalOpen(store))
                    {
                        if (const auto pixel =
                                workspacePixel(canvas))
                        {
                            input.sheetGestures.push_back(
                                SheetGesture{
                                    .kind = GestureKind::Press,
                                    .pixel = *pixel,
                                    .ink = down->button
                                           == MouseButton::Left});
                        }
                    }

                    continue;
                }

                if (down->button != MouseButton::Left)
                {
                    continue;
                }

                input.down = true;
                input.pressed = true;

                if (overMap(canvas) && !store.ui.pointerOverUi
                    && !store.ui.openMenu.has_value()
                    && !modalOpen(store))
                {
                    const auto signedCell =
                        signedCellUnder(canvas, store.camera);

                    if (insideMap(store, signedCell))
                    {
                        store.state.hovered = cellUnder(
                            store.state.map,
                            mapPoint(canvas, store.camera));
                    }

                    input.gestures.push_back(MapGesture{
                        .kind = GestureKind::Press,
                        .cell = cellUnder(
                            store.state.map,
                            mapPoint(canvas, store.camera)),
                        .signedCell = signedCell});
                }

                continue;
            }

            if (const auto *up =
                    std::get_if<PointerButtonReleased>(&event))
            {
                if (up->button == MouseButton::Middle)
                {
                    input.panning = false;
                    continue;
                }

                if (tilesView
                    && (up->button == MouseButton::Left
                        || up->button == MouseButton::Right))
                {
                    if (up->button == MouseButton::Left)
                    {
                        input.down = false;
                    }

                    input.sheetGestures.push_back(SheetGesture{
                        .kind = GestureKind::Release});
                    continue;
                }

                if (up->button != MouseButton::Left)
                {
                    continue;
                }

                input.down = false;
                input.gestures.push_back(
                    MapGesture{.kind = GestureKind::Release});
            }
        }
    }

}
