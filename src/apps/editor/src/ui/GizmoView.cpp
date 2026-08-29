#include "antwika/editor/ui/GizmoView.hpp"

#include <cstdint>
#include <string>

#include <antwika/camera/FlyCamera.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/input/MouseButton.hpp>

#include <antwika/render/Checkerboard.hpp>

#include "antwika/editor/ui/EditorLook.hpp"
#include "antwika/editor/ui/GizmoSheet.hpp"
#include "antwika/editor/ui/IconSheet.hpp"
#include "antwika/editor/ui/WidgetIds.hpp"

namespace antwika::editor
{

    namespace
    {
        [[nodiscard]] gfx::RectF getSheetRect(
            const ViewContext &viewContext)
        {
            return viewContext.workbench.sheetView.sheetRect.value_or(
                getIconSheetBounds(camera::kCanvasSize));
        }

        [[nodiscard]] gfx::RectF getDrawRect(
            const ViewContext &viewContext)
        {
            return viewContext.workbench.sheetView.canvasRect.value_or(
                getIconDrawBounds(camera::kCanvasSize));
        }
    }

    std::optional<std::size_t> GizmoView::getPickedIndex() const noexcept
    {
        return gizmoPicked;
    }

    void GizmoView::pick(const std::optional<std::size_t> gizmoIndex) noexcept
    {
        gizmoPicked = gizmoIndex;
    }

    void GizmoView::paint(
        const ViewContext &viewContext,
        const geometry::GridCell pixelCell,
        const bool erases)
    {
        auto &gizmos = viewContext.workbench.gizmos;

        setIconPixel(
            gizmos.sheetBitmap,
            *gizmoPicked,
            pixelCell,
            erases ? gfx::Color{.alpha = 0} : kWhiteColor);
        gizmos.texture =
            viewContext.render.viewportRenderer.createTexture(gizmos.sheetBitmap);
        gizmos.unsaved = true;
    }

    void GizmoView::drawSheet(const ViewContext &viewContext)
    {
        auto &viewportRenderer = viewContext.render.viewportRenderer;
        const auto &gizmos = viewContext.workbench.gizmos;

        if (!gizmoCheckerTexture)
        {
            gizmoCheckerTexture = viewportRenderer.createTexture(
                render::getCheckerboardBitmap(kIconCellSize, 4));
        }

        if (gizmos.texture == nullptr)
        {
            return;
        }

        const auto count = enums::kCount<GizmoKind>;
        const auto sheetRect = getSheetRect(viewContext);
        const auto drawRect = getDrawRect(viewContext);

        viewportRenderer.drawRect(sheetRect, kPanelColor);
        viewportRenderer.drawRect(drawRect, kPanelColor);

        {
            const auto sheetScope = viewportRenderer.clipScope(sheetRect);

            for (std::size_t index = 0; index < count; ++index)
            {
                const auto gizmoChosen = gizmoPicked == index;
                const auto place = antwika::editor::getIconCellRect(
                    sheetRect, count, index);

                viewportRenderer.drawTexture(
                    *gizmoCheckerTexture,
                    antwika::gfx::RectF(
                        {0.0F, 0.0F},
                        {static_cast<float>(kIconCellSize.width),
                         static_cast<float>(kIconCellSize.height)}),
                    place,
                    kWhiteColor);
                viewportRenderer.drawTexture(
                    *gizmos.texture,
                    antwika::editor::getIconSource(index),
                    place,
                    gizmoChosen ? kWhiteColor : kDisabledTintColor);

                drawOutline(
                    viewportRenderer,
                    place,
                    gizmoChosen ? kSelectionAccentColor : kGridLineColor);
            }
        }

        if (gizmoPicked.has_value() && *gizmoPicked < count)
        {
            const auto drawScope = viewportRenderer.clipScope(drawRect);
            const auto drawnAt =
                antwika::editor::getEditedIconRect(drawRect);

            viewportRenderer.drawTexture(
                *gizmoCheckerTexture,
                antwika::gfx::RectF(
                    {0.0F, 0.0F},
                    {static_cast<float>(kIconCellSize.width),
                     static_cast<float>(kIconCellSize.height)}),
                drawnAt,
                kWhiteColor);

            for (std::uint32_t row = 0; row < kIconCellSize.height; ++row)
            {
                for (std::uint32_t column = 0;
                     column < kIconCellSize.width;
                     ++column)
                {
                    const antwika::geometry::GridCell pixelCell{
                        column, row};

                    viewportRenderer.drawRect(
                        antwika::editor::getIconPixelRect(
                            drawnAt, pixelCell),
                        antwika::editor::getIconPixelColor(
                            gizmos.sheetBitmap, *gizmoPicked, pixelCell));
                }
            }
        }
    }

    bool GizmoView::claims(
        const View shownView, const bool playing) const noexcept
    {
        return !playing && shownView == View::Gizmos;
    }

    std::string GizmoView::getStatusText(const ViewContext &) const
    {
        return "6 gizmos - click a gizmo to take it up - "
               "lmb paints - rmb rubs out - a blank one shows "
               "as a white cross";
    }

    void GizmoView::draw(
        const ViewContext &viewContext, const ui::Frame &)
    {
        drawSheet(viewContext);
    }

    bool GizmoView::layoutRail(ui::Context &context)
    {
        const auto count = enums::kCount<GizmoKind>;

        if (!gizmoPicked.has_value() || *gizmoPicked >= count)
        {
            return false;
        }

        const auto gizmoPanel = context.column(
            antwika::ui::ContainerSpec{
                .widthSizing = antwika::ui::kGrowSizing,
                .backgroundColor = kPanelColor,
                .padding = kPanelPadding});

        panelTitle(context, "Gizmo");
        context.label(
            std::string(kGizmoNames.at(*gizmoPicked)), kTextColor);
        context.button(
            "clear",
            antwika::ui::ButtonSpec{
                .widgetId = antwika::editor::kGizmoClearWidget,
                .widthSizing = antwika::ui::kGrowSizing});

        return true;
    }

    bool GizmoView::takeWidgets(
        const ui::Interactions &interactions,
        const ViewContext &viewContext,
        std::optional<std::string> &)
    {
        if (interactions.activatedWidget != kGizmoClearWidget
            || !gizmoPicked.has_value()
            || *gizmoPicked >= enums::kCount<GizmoKind>)
        {
            return false;
        }

        auto &gizmos = viewContext.workbench.gizmos;

        wipeGizmo(gizmos.sheetBitmap, *gizmoPicked);
        gizmos.texture =
            viewContext.render.viewportRenderer.createTexture(gizmos.sheetBitmap);
        gizmos.unsaved = true;
        viewContext.document.markDirty();

        return true;
    }

    bool GizmoView::consumePress(
        const ViewContext &viewContext,
        const input::PointerButtonPressed &downPressed)
    {
        if (downPressed.button != input::MouseButton::Left
            && downPressed.button != input::MouseButton::Right)
        {
            return false;
        }

        const auto projectToScreen =
            viewContext.render.viewportRenderer.getViewport().toCanvas(
                gfx::Point{
                    .x = downPressed.position.x,
                    .y = downPressed.position.y});

        viewContext.workbench.pointer.pointerOnCanvas = gfx::PointF{
            static_cast<float>(projectToScreen.x),
            static_cast<float>(projectToScreen.y)};

        const auto count = enums::kCount<GizmoKind>;
        const auto chosenCell = iconCellAt(
            getSheetRect(viewContext),
            count,
            viewContext.workbench.pointer.pointerOnCanvas);

        if (chosenCell.has_value())
        {
            pick(
                downPressed.button == input::MouseButton::Right
                    ? std::nullopt
                    : std::optional{*chosenCell});

            return true;
        }

        if (!gizmoPicked.has_value() || *gizmoPicked >= count)
        {
            return true;
        }

        const auto pixel = iconPixelAt(
            getEditedIconRect(getDrawRect(viewContext)),
            viewContext.workbench.pointer.pointerOnCanvas);

        if (!pixel.has_value())
        {
            return true;
        }

        viewContext.workbench.stroke.erases =
            downPressed.button == input::MouseButton::Right;
        paint(viewContext, *pixel, viewContext.workbench.stroke.erases);
        viewContext.document.markDirty();
        viewContext.workbench.stroke.brushAtCell = pixel;
        viewContext.workbench.stroke.active = true;

        return true;
    }

    void GizmoView::trackPointer(const ViewContext &viewContext)
    {
        if (!viewContext.workbench.stroke.active
            || !gizmoPicked.has_value())
        {
            return;
        }

        const auto pixel = iconPixelAt(
            getEditedIconRect(getDrawRect(viewContext)),
            viewContext.workbench.pointer.pointerOnCanvas);

        if (!pixel.has_value())
        {
            return;
        }

        paint(viewContext, *pixel, viewContext.workbench.stroke.erases);
        viewContext.workbench.stroke.brushAtCell = pixel;
    }

}
