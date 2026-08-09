#include <algorithm>

#include "antwika/ui/Axis.hpp"
#include "antwika/ui/Context.hpp"
#include "antwika/ui/Scope.hpp"
#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/SplitSpec.hpp"

#include "Bar.hpp"
#include "Interactive.hpp"
#include "LayoutTree.hpp"
#include "Node.hpp"

namespace antwika::ui
{

    namespace
    {
        using detail::Bar;
        using detail::Interactive;
        using detail::Node;
        using detail::SplitInfo;

        [[nodiscard]] Sizing alongAxis(
            Axis axis, Axis wanted, Sizing thick, Sizing across) noexcept
        {
            return axis == wanted ? thick : across;
        }
    }

    Scope Context::split(const SplitSpec &spec)
    {
        const auto thick = fixedSize(themeValue.dividerThickness);

        const auto index = tree->open(Node{ // GCOVR_EXCL_LINE
            .axis = spec.axis,
            .width = spec.width,
            .height = spec.height,
            .gap = 0});

        const auto divider = tree->add(Node{ // GCOVR_EXCL_LINE
            .axis = spec.axis,
            .width = alongAxis(spec.axis, Axis::Row, thick, kGrow),
            .height = alongAxis(spec.axis, Axis::Column, thick, kGrow),
            .background = themeValue.divider,
            .id = spec.id,
            .style = Interactive{
                .idle = themeValue.divider,
                .hovered = themeValue.dividerHovered,
                .pressed = themeValue.dividerHeld}});

        tree->node(index).splitInfo = SplitInfo{
            .ratio = std::min(spec.ratio, kWholeSplit),
            .minimum = spec.minimum,
            .divider = divider};

        tree->addBar(Bar{
            .id = spec.id,
            .split = index,
            .divider = divider,
            .axis = spec.axis,
            .minimum = spec.minimum,
            .ratio = std::min(spec.ratio, kWholeSplit),
            .dragging = spec.dragging});

        return Scope{*this};
    }

}
