#include <array>
#include <cstdint>

#include <antwika/camera/FlyCamera.hpp>
#include <antwika/editor/editor/state/PanelSizes.hpp>
#include <antwika/editor/ui/EditorLook.hpp>

#include "antwika/editor/Editor.hpp"

#include "antwika/editor/ui/WidgetIds.hpp"

namespace antwika::editor
{

    namespace
    {
        struct PanelEdgeRow final
        {
            widget::WidgetId widget = widget::kNoWidget;

            std::uint32_t PanelSizes::*extent = nullptr;
        };

        constexpr std::array<PanelEdgeRow, 7> kPanelEdgeRows{
            PanelEdgeRow{kToolPanelEdgeWidget, &PanelSizes::toolWidth},
            PanelEdgeRow{kEntityListEdgeWidget, &PanelSizes::entityWidth},
            PanelEdgeRow{kDrawColumnEdgeWidget, &PanelSizes::inspectWidth},
            PanelEdgeRow{kRailEdgeWidget, &PanelSizes::railWidth},
            PanelEdgeRow{kPlanFirstEdgeWidget, &PanelSizes::planFirstWidth},
            PanelEdgeRow{kPlanSecondEdgeWidget, &PanelSizes::planSecondWidth},
            PanelEdgeRow{kPlanDetailEdgeWidget, &PanelSizes::cardWidth}};
    }

    bool Editor::beginEdgeDrag(const ui::Interactions &interactions)
    {
        if (!interactions.edge.has_value())
        {
            return false;
        }

        for (const auto &row : kPanelEdgeRows)
        {
            if (interactions.edge->edgeWidget != row.widget)
            {
                continue;
            }

            preferences.panelSizes.*row.extent = interactions.edge->extent;
            pointer.heldEdgeWidget = row.widget;

            return true;
        }

        return false;
    }

    void Editor::endEdgeDrag()
    {
        pointer.heldEdgeWidget = widget::kNoWidget;
    }

    std::uint32_t Editor::panelWidthOf(
        std::uint32_t PanelSizes::*extent,
        const std::uint32_t restingWidth) const
    {
        return getFittedPanelWidth(
            preferences.panelSizes.*extent,
            restingWidth,
            viewportRenderer.getWindowSize().width);
    }

    float Editor::getRailWidthOnCanvas() const
    {
        return antwika::editor::getRailWidthOnCanvas(
            preferences.panelSizes,
            viewportRenderer.getWindowSize(),
            camera::kCanvasSize);
    }

}
