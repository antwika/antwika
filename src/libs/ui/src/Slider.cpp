#include <cstdint>

#include "antwika/ui/Axis.hpp"
#include "antwika/ui/Context.hpp"
#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/SliderSpec.hpp"

#include "FocusRing.hpp"
#include "LayoutTree.hpp"
#include "Node.hpp"
#include "ScrollBar.hpp"

namespace antwika::ui
{

    namespace
    {
        using detail::FocusRing;
        using detail::Node;
        using detail::ScrollBar;
    }

    void Context::slider(const SliderSpec &spec)
    {
        const FocusRing ring{
            .color = themeValue.focusRingColor,
            .thickness = themeValue.focusRingThickness};

        const auto track = tree->open(Node{ // GCOVR_EXCL_LINE
            .axis = Axis::Row,
            .widthSizing = spec.widthSizing,
            .heightSizing = getFixedSize(themeValue.sliderHeight),
            .gap = 0,
            .backgroundColor = themeValue.scrollTrackColor,
            .widgetId = spec.widgetId,
            .focusStyle = ring});

        const auto thumb = tree->add(Node{ // GCOVR_EXCL_LINE
            .widthSizing = getFixedSize(0),
            .heightSizing = kGrowSizing,
            .backgroundColor = themeValue.scrollThumbColor});

        closeContainer();

        tree->addRail(ScrollBar{
            .widgetId = spec.widgetId,
            .track = track,
            .thumb = thumb,
            .value = spec.value,
            .range = spec.range,
            .dragging = spec.dragging});
    }

}
