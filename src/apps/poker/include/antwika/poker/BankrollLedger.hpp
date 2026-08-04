#pragma once

#include <map>
#include <string>

#include <antwika/holdem/Chips.hpp>

namespace antwika::poker
{

    using antwika::holdem::Chips;

    /**
     * @brief Every player's money, tracked outside any single game.
     *
     * Deliberately knows nothing about tables, seats or hands: chips at
     * a table are on loan from here, and a player's standing across a
     * session is the balance left once they have cashed out. Keeping the
     * two apart is what makes "never buy in for more than you have"
     * something the ledger can enforce on its own.
     */
    class BankrollLedger final
    {
    public:
        /**
         * @brief Add money to a player's bankroll.
         *
         * A name that has never been seen is opened on first deposit.
         * @param player The player's name.
         * @param amount Chips to add.
         */
        void deposit(const std::string &player, Chips amount);

        /**
         * @brief Take money out of a player's bankroll.
         * @param player The player's name.
         * @param amount Chips to remove.
         * @throws BankrollError If the player's balance is smaller than
         * amount, an unknown name included.
         */
        void withdraw(const std::string &player, Chips amount);

        /**
         * @brief Read one player's balance.
         * @param player The player's name.
         * @return Their balance, or zero for a name never deposited to.
         */
        [[nodiscard]] Chips balanceOf(const std::string &player) const;

        /**
         * @brief Read every balance.
         * @return Balances by player name, in name order.
         */
        /**
         * @brief Stand every balance at a remembered value.
         *
         * Wholesale, since a restore is resuming a session rather
         * than merging two -- SessionStore::restore's rule.
         *
         * @param accounts The balances to hold instead.
         */
        void restore(std::map<std::string, Chips> accounts);

        [[nodiscard]] const std::map<std::string, Chips> &balances()
            const noexcept;

    private:
        std::map<std::string, Chips> accounts;
    };

} // namespace antwika::poker
