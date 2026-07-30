#include "antwika/poker/BankrollLedger.hpp"

#include <string>

#include "antwika/poker/BankrollError.hpp"

namespace antwika::poker
{

    void BankrollLedger::deposit(const std::string &player, Chips amount)
    {
        accounts[player] += amount;
    }

    void BankrollLedger::withdraw(const std::string &player, Chips amount)
    {
        const auto account = accounts.find(player);
        if (account == accounts.end() || account->second < amount)
        {
            throw BankrollError(
                "BankrollLedger: " + player
                + " does not hold that much");
        }
        account->second -= amount;
    }

    Chips BankrollLedger::balanceOf(const std::string &player) const
    {
        const auto account = accounts.find(player);
        if (account == accounts.end())
        {
            return 0;
        }
        return account->second;
    }

    const std::map<std::string, Chips> &BankrollLedger::balances()
        const noexcept
    {
        return accounts;
    }

} // namespace antwika::poker
