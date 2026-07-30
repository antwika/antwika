#pragma once

namespace antwika::game
{

    /**
     * @brief Marks an entity's cell as walkable.
     *
     * A tag: it carries nothing, because being a path is the whole fact.
     * antwika::ecs::Component asks only for trivial copyability and
     * standard layout, both of which an empty struct has.
     *
     * The cell it refers to is a separate Cell component, so the two can
     * be viewed together and a walker can be at a cell without being one.
     */
    struct Path
    {
        /**
         * @brief Compare two path tags.
         * @param other The tag to compare against.
         * @return Always true: the type carries no state.
         */
        [[nodiscard]] bool operator==(const Path &other) const = default;
    };

} // namespace antwika::game
