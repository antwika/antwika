#pragma once

#include <functional>
#include <optional>

#include <antwika/geometry/Size.hpp>
#include <antwika/log/ILogger.hpp>

namespace antwika::replay
{

    /**
     * @brief What a caller is about to lay its input out against, and
     * where to say so when the replay it is loading disagrees.
     *
     * A mismatch is a warning and never an error.
     * Refusing the load would break every recording made before the
     * canvas was written into the format, and the caller may well know
     * better than the file does -- so this reports rather than decides.
     */
    struct CanvasCheck
    {
        /**
         * @brief The canvas the loaded events will be resolved against.
         *
         * Unset means the caller has nothing to compare, so nothing is
         * compared.
         */
        std::optional<geometry::Size> canvas{};

        /**
         * @brief Where a mismatch is reported.
         *
         * Unset means a mismatch goes unreported.
         * That is the price of a library that owns no logger of its own,
         * and it keeps a check with nowhere to report to from being a
         * reason to reach for a global one.
         */
        std::optional<std::reference_wrapper<log::ILogger>> logger{};
    };

} // namespace antwika::replay
