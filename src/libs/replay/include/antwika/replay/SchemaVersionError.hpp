#pragma once

#include <antwika/replay/ReplayFormatError.hpp>

namespace antwika::replay
{

    /**
     * @brief Thrown when a document's schema version is unreadable, or
     * is one this build cannot reach the current version from.
     *
     * That covers a version member that is not a non-negative integer, a
     * version newer than this build knows about, and a gap in the
     * migration chain -- no IMigration from some version N to N+1 -- all
     * of which mean the same thing to a caller: this document cannot be
     * brought to a shape this build understands.
     *
     * A subclass of ReplayFormatError rather than a sibling of it, since
     * a document this build cannot read *is* a document this build
     * cannot read; every existing `catch (const ReplayFormatError &)`
     * keeps working, and a caller that wants to tell "your file is from
     * a newer release" apart from "your file is corrupt" catches this
     * narrower type first.
     * The message always names the version found and what this build
     * supports, since that is the only thing a person can act on.
     */
    class SchemaVersionError final : public ReplayFormatError
    {
    public:
        using ReplayFormatError::ReplayFormatError;
    };

} // namespace antwika::replay
