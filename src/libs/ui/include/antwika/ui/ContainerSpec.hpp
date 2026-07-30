#pragma once

#include <cstdint>
#include <optional>

#include <antwika/gfx/Color.hpp>

#include "antwika/ui/Alignment.hpp"
#include "antwika/ui/Sizing.hpp"

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
    };

} // namespace antwika::ui
