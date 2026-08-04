#pragma once

#include <cstdint>
#include <string_view>

namespace antwika::config
{

    /**
     * @brief What one application's config format states about itself.
     *
     * The magic is what tells this document kind apart from every
     * other persisted format -- they all state their version in the
     * same member, so the magic is the only thing distinguishing a
     * config handed the wrong file from a config with every member
     * missing. The version is the revision the application currently
     * writes, on docs/schema-versioning.md's terms.
     *
     * A plain value: an application declares one beside its magic and
     * version constants and hands it to the functions in
     * ConfigDocument.hpp. The magic is a view because it always names
     * a constant the application owns; nothing here copies or keeps
     * it.
     */
    struct Format
    {
        /** @brief What every document of the format says it is. */
        std::string_view magic;

        /** @brief The revision the application currently writes. */
        std::uint32_t version;
    };

} // namespace antwika::config
