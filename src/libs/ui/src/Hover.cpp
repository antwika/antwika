#include "antwika/ui/Hover.hpp"

#include <variant>

#include <antwika/gfx/Color.hpp>

#include "antwika/ui/DrawCommand.hpp"
#include "antwika/ui/HoverTarget.hpp"
#include "antwika/ui/WidgetId.hpp"

#include "Contains.hpp"

namespace antwika::ui
{

    namespace
    {
        using antwika::gfx::Color;

        void recolour(
            DrawList &drawList,
            const HoverTarget &target,
            Color color) noexcept
        {
            if (target.command >= drawList.size())
            {
                return;
            }

            if (auto *fill =
                    std::get_if<FillRect>(&drawList[target.command]))
            {
                fill->color = color;
            }
        }

        [[nodiscard]] bool lit(
            const HoverTarget &target, const HoverTarget *underTarget) noexcept
        {
            if (underTarget == nullptr)
            {
                return false;
            }

            return &target == underTarget
                   || (target.widgetId != kNoWidget
                       && target.widgetId == underTarget->widgetId);
        }
    }

    void applyHover(
        DrawList &drawList,
        const HoverTargets &targets,
        HoverPointer hover)
    {
        if (!hover.positionPoint)
        {
            return;
        }

        const HoverTarget *underTarget = nullptr;

        for (const auto &target : targets)
        {
            if (!target.held
                && detail::contains(target.rect, *hover.positionPoint))
            {
                underTarget = &target;
            }
        }

        for (const auto &target : targets)
        {
            if (target.held)
            {
                continue;
            }

            recolour(
                drawList,
                target,
                lit(
                    target,
                    underTarget) ? target.hoveredColor : target.idleColor);
        }
    }

}
