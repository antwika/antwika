#pragma once

#include "antwika/game/BuildTool.hpp"

namespace antwika::game
{

    /**
     * @brief Marks an entity's cell as built on, and says with what.
     *
     * The tool that placed it is kept rather than a kind of its own, so
     * the palette button, the placement and the picture cannot name three
     * different things.
     *
     * The cell it stands on is a separate Cell component, the same way a
     * Path's is, so the two can be viewed together.
     */
    struct Building
    {
        BuildTool kind = BuildTool::House;

        /**
         * @brief Compare two buildings.
         * @param other The building to compare against.
         * @return True when both were placed by the same tool.
         */
        [[nodiscard]] bool operator==(const Building &other) const = default;
    };

} // namespace antwika::game
