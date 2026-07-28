#pragma once

#include <cstdint>

namespace antwika::scheduler
{

    /**
     * @brief A job's scheduling priority: a plain integer, never
     * reinterpreted as anything else.
     *
     * A scoped enum with no enumerators over std::uint8_t, mirroring
     * antwika::ecs::Entity's exact idiom, same as JobId. Higher raw
     * value runs first. Priority is not closed to just the named
     * constants below; any std::uint8_t is a valid priority, the named
     * constants are just the common cases.
     */
    enum class Priority : std::uint8_t
    {
    };

    /**
     * @brief The lowest of the named priority constants.
     */
    inline constexpr Priority kLowPriority{0};

    /**
     * @brief The default, everyday priority constant.
     */
    inline constexpr Priority kNormalPriority{1};

    /**
     * @brief A priority above kNormalPriority.
     */
    inline constexpr Priority kHighPriority{2};

    /**
     * @brief The highest of the named priority constants.
     */
    inline constexpr Priority kCriticalPriority{3};

    /**
     * @brief Get the raw integer value backing a priority.
     * @param priority The priority to unwrap.
     * @return The underlying std::uint8_t value.
     */
    [[nodiscard]] constexpr std::uint8_t rawValue(Priority priority) noexcept
    {
        return static_cast<std::uint8_t>(priority);
    }

} // namespace antwika::scheduler
