#include "antwika/ui/Axis.hpp"
#include "antwika/ui/Context.hpp"
#include "antwika/ui/EdgeSpec.hpp"
#include "antwika/ui/Sizing.hpp"

#include "LayoutTree.hpp"
#include "Node.hpp"
#include "PanelEdge.hpp"
#include "StateColors.hpp"

namespace antwika::ui
{

    namespace
    {
        using detail::Node;
        using detail::PanelEdge;
        using detail::StateColors;
    }

    void Context::edge(const EdgeSpec &spec)
    {
        const auto axis = tree->getNode(tree->getOpenIndex()).axis;
        const auto thickness = getFixedSize(themeValue.dividerThickness);

        Node barNode{ // GCOVR_EXCL_LINE
            .axis = axis,
            .backgroundColor = themeValue.dividerColor,
            .widgetId = spec.widgetId,
            .styleColors = StateColors{
                .idleColor = themeValue.dividerColor,
                .hoveredColor = themeValue.dividerHoveredColor,
                .pressedColor =
                    themeValue.dividerHeldColor}}; // GCOVR_EXCL_LINE

        if (axis == Axis::Row)
        {
            barNode.widthSizing = thickness;
            barNode.heightSizing = kGrowSizing;
        }
        else
        {
            barNode.widthSizing = kGrowSizing;
            barNode.heightSizing = thickness;
        }

        const auto bar = tree->add(std::move(barNode));

        tree->addEdge(PanelEdge{
            .widgetId = spec.widgetId,
            .panelWidget = spec.panelWidget,
            .bar = bar,
            .axis = axis,
            .minimum = spec.minimum,
            .maximum = spec.maximum,
            .dragging = spec.dragging});
    }

}
