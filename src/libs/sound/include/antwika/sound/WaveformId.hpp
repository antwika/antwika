#pragma once

#include <cstdint>

namespace antwika::sound
{

    /**
     * @brief Names one waveform a library is holding.
     *
     * An empty scoped enum over its backing integer rather than a plain
     * alias, following scheduler::JobId: an id is a handle, and a handle
     * that is implicitly an integer is a handle that gets arithmetic
     * done to it.
     */
    enum class WaveformId : std::uint32_t
    {
    };

    /**
     * @brief Get an id's underlying number.
     * @param id The id to unwrap.
     * @return Its value.
     */
    [[nodiscard]] constexpr std::uint32_t rawValue(WaveformId id) noexcept
    {
        return static_cast<std::uint32_t>(id);
    }

} // namespace antwika::sound
