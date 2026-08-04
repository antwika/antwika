#pragma once

#include <stdexcept>

namespace antwika::config
{

    /**
     * @brief Thrown when a config document cannot be read.
     *
     * That covers a document that is not JSON at all, one that is not
     * the expected format's, a version the reading build cannot reach
     * the current one from, a member the schema refuses, and a file
     * that is there but cannot be read.
     *
     * Refused rather than repaired, on the terms every persisted format
     * here holds to: a config with a member quietly clamped or
     * defaulted is a game nobody asked for, and a rebalance that only
     * half took effect would surface as a run drifting from the
     * numbers its author wrote.
     *
     * One type for every application's config, declared here because
     * this module owns the failure category -- "a config file could
     * not be read" -- exactly as ReplayFormatError covers every
     * application's replays. An application's *save* or *options* file
     * keeps an error type of its own, since those are different
     * categories of document.
     */
    class ConfigFormatError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

} // namespace antwika::config
