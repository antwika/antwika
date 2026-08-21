#include <algorithm>

#include "antwika/ui/Axis.hpp"
#include "antwika/ui/Context.hpp"
#include "antwika/ui/ContainerScope.hpp"
#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/SplitSpec.hpp"

#include "Splitter.hpp"
#include "StateColors.hpp"
#include "LayoutTree.hpp"
#include "Node.hpp"

namespace antwika::ui
{

    namespace
    {
        using detail::Splitter;
        using detail::StateColors;
        using detail::Node;
        using detail::SplitInfo;

        [[nodiscard]] Sizing alongAxis(
            Axis axis,
            Axis wantedAxis,
            Sizing thickSizing,
            Sizing acrossSizing) noexcept
        {
            return axis == wantedAxis ? thickSizing : acrossSizing;
        }
    }

    ContainerScope Context::split(const SplitSpec &spec)
    {
        const auto thickness = fixedSize(themeValue.dividerThickness);

        const auto index = tree->open(Node{ // GCOVR_EXCL_LINE
            .axis = spec.axis,
            .widthSizing = spec.widthSizing,
            .heightSizing = spec.heightSizing,
            .gap = 0});

        const auto divider = tree->add(Node{ // GCOVR_EXCL_LINE
            .axis = spec.axis,
            .widthSizing = alongAxis(spec.axis, Axis::Row, thickness,
            kGrowSizing),
            .heightSizing = alongAxis(
                spec.axis,
                Axis::Column,
                thickness,
                kGrowSizing),
            .backgroundColor = themeValue.dividerColor,
            .widgetId = spec.widgetId,
            .styleColors = StateColors{
                .idleColor = themeValue.dividerColor,
                .hoveredColor = themeValue.dividerHoveredColor,
                .pressedColor = themeValue.dividerHeldColor}});

        tree->node(index).splitInfo = SplitInfo{
            .ratio = std::min(spec.ratio, kSplitRatioScale),
            .minimum = spec.minimum,
            .divider = divider};

        tree->addBar(Splitter{
            .widgetId = spec.widgetId,
            .split = index,
            .divider = divider,
            .axis = spec.axis,
            .minimum = spec.minimum,
            .ratio = std::min(spec.ratio, kSplitRatioScale),
            .dragging = spec.dragging});

        return ContainerScope{*this};
    }

}
