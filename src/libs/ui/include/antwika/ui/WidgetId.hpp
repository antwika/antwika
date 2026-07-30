#pragma once

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

} // namespace antwika::ui
