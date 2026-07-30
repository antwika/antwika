#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>

namespace antwika::ui
{

    /**
     * @brief Identifies a widget a pointer can land on.
     *
     * A scoped enum with no enumerators over std::uint64_t, following
     * antwika::gfx::WindowId and antwika::ecs::Entity.
     *
     * The caller chooses the values, and only has to keep them distinct
     * among the widgets one frame declares. Declaration order is
     * deliberately not used instead: an index shifts the moment a layout
     * gains a conditional widget, and this is the value that crosses back
     * into application state, so it has to keep meaning the same widget.
     *
     * Two widgets sharing an id is legal and means they are the same
     * widget: the topmost one is hovered and activated, and both take the
     * resolved appearance.
     */
    enum class WidgetId : std::uint64_t
    {
    };

    /**
     * @brief The id of a widget nothing can point at.
     *
     * What a widget the caller did not name carries, and what an
     * Interactions field holds when nothing was hovered or activated.
     */
    inline constexpr WidgetId kNoWidget{0};

    /**
     * @brief Check that a frame's widget ids are all different.
     *
     * Two widgets sharing an id is legal and means they are one widget,
     * so a caller that did not mean that gets a plausible wrong answer
     * and no diagnostic. Named ids are constants, so the mistake can be
     * a build error instead:
     *
     * @code
     * static_assert(
     *     assertDistinct(kZoomOut, kZoomIn, kResetView),
     *     "every toolbar widget needs its own id");
     * @endcode
     *
     * Says nothing about kNoWidget: an id list is free to contain it,
     * and a widget carrying it is simply one nothing can point at.
     *
     * Compares every pair rather than sorting, since a frame declares a
     * handful of ids and the quadratic loop is the one a constant
     * evaluator can run without a copy.
     *
     * @param ids The ids one frame declares, in any order.
     * @return True when no two of them are equal; vacuously true for
     * none and for one.
     */
    template <std::same_as<WidgetId>... Ids>
    [[nodiscard]] constexpr bool assertDistinct(Ids... ids) noexcept
    {
        const std::array<WidgetId, sizeof...(Ids)> values{ids...};

        for (std::size_t index = 0; index < values.size(); ++index)
        {
            for (std::size_t other = index + 1; other < values.size();
                 ++other)
            {
                if (values[index] == values[other])
                {
                    return false;
                }
            }
        }

        return true;
    }

} // namespace antwika::ui
