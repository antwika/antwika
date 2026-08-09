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
            DrawList &commands,
            const HoverTarget &target,
            Color color) noexcept
        {
            if (target.command >= commands.size())
            {
                return;
            }

            if (auto *fill =
                    std::get_if<FillRect>(&commands[target.command]))
            {
                fill->color = color;
            }
        }

        [[nodiscard]] bool lit(
            const HoverTarget &target, const HoverTarget *under) noexcept
        {
            if (under == nullptr)
            {
                return false;
            }

            return &target == under
                   || (target.id != kNoWidget
                       && target.id == under->id);
        }
    }

    void applyHover(
        DrawList &commands,
        const HoverTargets &targets,
        HoverPointer hover)
    {
        if (!hover.position)
        {
            return;
        }

        const HoverTarget *under = nullptr;

        for (const auto &target : targets)
        {
            if (!target.held && detail::contains(target.rect, *hover.position))
            {
                under = &target;
            }
        }

        for (const auto &target : targets)
        {
            if (target.held)
            {
                continue;
            }

            recolour(
                commands,
                target,
                lit(target, under) ? target.hovered : target.idle);
        }
    }

}
