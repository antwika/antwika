#pragma once

#include <cstdint>

namespace antwika::gfx
{

    /**
     * @brief Identifies a window within one backend.
     *
     * A scoped enum with no enumerators over std::uint64_t, following
     * antwika::ecs::Entity and antwika::scheduler::JobId: comparable like
     * a raw integer, but impossible to mix up with one by accident.
     *
     * A backend chooses the values, and only has to keep them distinct
     * among its own live windows. A framework that already numbers its
     * windows, as SDL does, can hand its own ids straight through.
     */
    enum class WindowId : std::uint64_t
    {
    };

    /**
     * @brief The window id that never identifies a real window.
     */
    inline constexpr WindowId kNullWindowId{0};

    /**
     * @brief Get the raw integer value backing a window id.
     * @param id The id to unwrap.
     * @return The underlying std::uint64_t value.
     */
    [[nodiscard]] constexpr std::uint64_t rawValue(WindowId id) noexcept
    {
        return static_cast<std::uint64_t>(id);
    }

} // namespace antwika::gfx
