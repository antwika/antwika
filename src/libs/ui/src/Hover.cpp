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

        /**
         * @brief Recolour the fill a target names, if that is what it
         * names.
         *
         * A draw list and a list of targets are two values a caller
         * pairs, so a target may name a command that is not there or one
         * that is not a fill. Both leave the picture alone: this is
         * appearance, and refusing to draw is a smaller failure than
         * throwing out of one.
         *
         * @param commands The picture, written in place.
         * @param target The widget whose fill is being recoloured.
         * @param color The colour to give it.
         */
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

        /**
         * @brief Decide whether one target shows its hovered colour.
         *
         * The frontmost target the pointer is inside shows it, and so
         * does every other target carrying that target's id: two nodes
         * sharing an id are one widget, which is the rule resolve()'s
         * dressing pass already follows by comparing ids rather than
         * geometry. Lighting only the frontmost would make the same
         * hover resolve differently through the two routes.
         *
         * An unnamed target is only ever the frontmost one, since
         * comparing two kNoWidgets would call every unnamed widget the
         * same one.
         *
         * @param target The target being decided.
         * @param under The frontmost target the pointer is inside, or
         * null when it is inside none.
         * @return True when it shows its hovered colour.
         */
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
    } // namespace

    void applyHover(
        DrawList &commands,
        const HoverTargets &targets,
        HoverPointer hover)
    {
        if (!hover.position)
        {
            return;
        }

        // Paint order, so the last one hit is the frontmost.
        // resolve() reads the same order backwards to say the same thing.
        const HoverTarget *under = nullptr;

        for (const auto &target : targets)
        {
            if (!target.held && detail::contains(target.rect, *hover.position))
            {
                under = &target;
            }
        }

        // Every target is decided, not only the one under the pointer.
        // Otherwise whatever the recorded stream last hovered stays lit.
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

} // namespace antwika::ui
