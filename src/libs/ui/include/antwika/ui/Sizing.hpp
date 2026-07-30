#pragma once

#include <cstdint>

namespace antwika::ui
{

    /**
     * @brief How a widget's extent on one axis is decided.
     */
    enum class SizeMode : std::uint8_t
    {
        /**
         * @brief Exactly the pixel count given alongside it.
         */
        Fixed = 0,

        /**
         * @brief Just large enough for the content.
         */
        Fit,

        /**
         * @brief The content's size, plus a share of whatever is left
         * over in the container.
         */
        Grow,
    };

    /**
     * @brief One widget's requested extent on one axis.
     *
     * Fit and Grow ask for the same thing while a container is measuring
     * itself: a growing child still reports its content as a minimum, so
     * that a container fitting around growing children does not collapse
     * to its padding.
     * The two differ only when a container has space left over to hand
     * out.
     */
    struct Sizing
    {
        SizeMode mode = SizeMode::Fit;
        std::uint32_t pixels = 0;

        /**
         * @brief Compare two requests.
         * @param other The request to compare against.
         * @return True when the mode and the pixel count both match.
         */
        [[nodiscard]] bool operator==(const Sizing &other) const = default;
    };

    /**
     * @brief Ask for exactly enough room for the content.
     */
    inline constexpr Sizing kFit{.mode = SizeMode::Fit};

    /**
     * @brief Ask for the content's room plus a share of what is left.
     */
    inline constexpr Sizing kGrow{.mode = SizeMode::Grow};

    /**
     * @brief Ask for an exact number of pixels.
     * @param pixels The extent to ask for.
     * @return The request.
     */
    [[nodiscard]] constexpr Sizing fixedSize(std::uint32_t pixels) noexcept
    {
        return Sizing{.mode = SizeMode::Fixed, .pixels = pixels};
    }

} // namespace antwika::ui
