#pragma once

#include "antwika/holdem/Action.hpp"
#include "antwika/holdem/TableView.hpp"

namespace antwika::holdem
{

    /**
     * @brief Something that can be asked what to do with a poker hand.
     *
     * The one seam between the rules of hold'em and any opinion about
     * how to play it: a bot, a scripted line of play, or a prompt to a
     * person all sit behind this and the table cannot tell them apart.
     */
    class IAgent
    {
    public:
        virtual ~IAgent() = default;

        /**
         * @brief Decide what to do with the hand in front of you.
         * @param view Everything this player is entitled to know.
         * @return The chosen action, which must be legal for that view.
         */
        [[nodiscard]] virtual Action act(const TableView &view) = 0;
    };

} // namespace antwika::holdem
