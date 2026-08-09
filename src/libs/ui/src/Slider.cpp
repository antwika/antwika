#include <cstdint>

#include "antwika/ui/Axis.hpp"
#include "antwika/ui/Context.hpp"
#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/SliderSpec.hpp"

#include "FocusRing.hpp"
#include "LayoutTree.hpp"
#include "Node.hpp"
#include "Rail.hpp"

namespace antwika::ui
{

    namespace
    {
        using detail::FocusRing;
        using detail::Node;
        using detail::Rail;
    }

    void Context::slider(const SliderSpec &spec)
    {
        const FocusRing ring{
            .color = themeValue.focusRing,
            .thickness = themeValue.focusRingThickness};

        const auto track = tree->open(Node{ // GCOVR_EXCL_LINE
            .axis = Axis::Row,
            .width = spec.width,
            .height = fixedSize(themeValue.sliderHeight),
            .gap = 0,
            .background = themeValue.scrollTrack,
            .id = spec.id,
            .focusStyle = ring});

        const auto thumb = tree->add(Node{ // GCOVR_EXCL_LINE
            .width = fixedSize(0),
            .height = kGrow,
            .background = themeValue.scrollThumb});

        closeContainer();

        tree->addRail(Rail{
            .id = spec.id,
            .track = track,
            .thumb = thumb,
            .value = spec.value,
            .range = spec.range,
            .dragging = spec.dragging});
    }

}
