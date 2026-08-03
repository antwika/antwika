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

    /**
     * @brief The colour the fire risk is written in.
     *
     * The risks stopped answering to services, so their inks stopped
     * being serviceColour()'s -- but a risk line and a map view of the
     * same risk still have to agree, which is why each ink is named
     * once here rather than in whichever file paints it.
     */
    inline constexpr Color kFireRiskInk{
        .red = 224, .green = 148, .blue = 78};

    /** @brief The colour the collapse risk is written in. */
    inline constexpr Color kCollapseRiskInk{
        .red = 170, .green = 176, .blue = 188};

    /**
     * @brief The colour the disease risk is written in.
     *
     * The medicine's own ink, since medicine is what holds the disease
     * off: the risk line and the amount line that answers it read as a
     * pair.
     */
    inline constexpr Color kDiseaseRiskInk{
        .red = 214, .green = 120, .blue = 148};

} // namespace antwika::game
