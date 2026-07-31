#pragma once

#include <cstdint>
#include <optional>

#include <antwika/gfx/Color.hpp>

#include "antwika/ui/Alignment.hpp"
#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui
{

    using antwika::gfx::Color;

    /**
     * @brief What one container is being asked for.
     *
     * Every field is defaulted, so an unremarkable container is `{}` and
     * a caller writes only what it cares about.
     *
     * The three optionals are what let a widget say "whatever the theme
     * says" without repeating the theme at every call site, and what lets
     * a panel differ from a column purely in what an unset field means.
     */
    struct ContainerSpec
    {
        /**
         * @brief How wide, defaulting to filling the room across.
         */
        Sizing width = kGrow;

        /**
         * @brief How tall, defaulting to fitting the content.
         */
        Sizing height = kFit;

        /**
         * @brief Where the children sit across this container's axis.
         */
        Alignment cross = Alignment::Start;

        /**
         * @brief The fill behind the children, if any.
         */
        std::optional<Color> background{};

        /**
         * @brief The inset on every side, if not the default.
         */
        std::optional<std::uint32_t> padding{};

        /**
         * @brief The space between children, if not the theme's.
         */
        std::optional<std::uint32_t> gap{};

        /**
         * @brief What to call this container, if it needs a name.
         *
         * Left unset, the container is anonymous, which is what every
         * container was before this existed and what an ordinary row or
         * column still is.
         *
         * Naming one puts its arranged area in Frame::rects, which is
         * how an application places its own art against the layout
         * rather than beside it.
         *
         * **It also makes the container something the pointer can land
         * on**, because that is what an id means here: it is reported
         * through Interactions::hovered and Interactions::activated like
         * any other named widget.
         * A named container is still not a stop in the tab order and
         * still draws no appearance of its own -- only a widget carrying
         * a focus ring is focusable, and only one carrying interactive
         * colours is dressed -- so naming one changes nothing about how
         * it looks.
         * A child declared inside it sits at a higher index and so wins
         * the hit-test, which is why a named panel does not swallow the
         * buttons on it.
         */
        WidgetId id = kNoWidget;
    };

} // namespace antwika::ui
