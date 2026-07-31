#pragma once

namespace antwika::ecs_commons
{

    /**
     * @brief A component that carries nothing but its own type.
     *
     * A tag answers a yes/no question about an entity by existing, so it
     * has no fields.
     * What distinguishes one tag from another is the Kind it is stamped
     * with, since a World keys its storage by component type: an app
     * writes
     *
     *     using Path = Tag<struct PathKind>;
     *     using Blocked = Tag<struct BlockedKind>;
     *
     * and gets two unrelated storages out of one template, without either
     * of them being reachable through the other.
     * Kind is never defined and never instantiated -- it exists only to
     * be a distinct type -- which is why an incomplete struct declared
     * inline in the alias is enough.
     *
     * @tparam Kind The tag's identity. Need not be a complete type.
     */
    template <typename Kind>
    struct Tag
    {
        /**
         * @brief Compare two tags.
         * @param other The tag to compare against.
         * @return Always true: the type carries no state.
         */
        [[nodiscard]] bool operator==(const Tag &other) const = default;
    };

} // namespace antwika::ecs_commons
