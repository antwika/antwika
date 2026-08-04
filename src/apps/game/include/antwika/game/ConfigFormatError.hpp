#pragma once

#include <stdexcept>

namespace antwika::game
{

    /**
     * @brief Thrown when a config document is not one this app can read.
     *
     * That covers a document that is not JSON at all, one that is not
     * this format's, a version this build cannot reach the current one
     * from, a member of the wrong shape, a period of zero ticks, and a
     * file that is there but cannot be read.
     *
     * Refused rather than repaired, for SaveGame's reason: a config
     * with a member quietly clamped or defaulted is a game nobody
     * asked for, and a rebalance that only half took effect would
     * surface as a city drifting from the numbers its author wrote.
     *
     * Its own type rather than OptionsFormatError or SaveFormatError,
     * because a bad config is neither a layout nor a session, and the
     * house rule is one exception type per failure category.
     */
    class ConfigFormatError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

} // namespace antwika::game
