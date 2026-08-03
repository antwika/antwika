#pragma once

#include <antwika/gfx/Color.hpp>

#include "antwika/game/Resource.hpp"
#include "antwika/game/Service.hpp"

namespace antwika::game
{

    using antwika::gfx::Color;

    /**
     * @brief Get the colour a resource is written in.
     *
     * The one crossing between a resource and a colour, so two things
     * naming the same good cannot come out in two different greens.
     *
     * It lives here rather than in Resource.hpp because that header is
     * the simulation's vocabulary and a colour is a fact about the
     * picture; a gfx type in there would put a render concern into the
     * component every building carries.
     *
     * @param resource The resource to colour.
     * @return Its colour, opaque.
     */
    [[nodiscard]] Color resourceColour(Resource resource) noexcept;

    /**
     * @brief Get the colour a service is written in.
     *
     * resourceColour()'s counterpart, here for its reason: the one
     * crossing between a service and a colour, so a coverage line and
     * whatever else ever names the same service cannot come out in two
     * different blues.
     *
     * @param service The service to colour.
     * @return Its colour, opaque.
     */
    [[nodiscard]] Color serviceColour(Service service) noexcept;

} // namespace antwika::game
