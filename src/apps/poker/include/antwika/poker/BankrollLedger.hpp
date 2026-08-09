#pragma once

#include <map>
#include <string>

#include <antwika/holdem/Chips.hpp>

namespace antwika::poker
{

    using antwika::holdem::Chips;

    class BankrollLedger final
    {
    public:
        void deposit(const std::string &player, Chips amount);

        void withdraw(const std::string &player, Chips amount);

        [[nodiscard]] Chips balanceOf(const std::string &player) const;

        void restore(std::map<std::string, Chips> accounts);

        [[nodiscard]] const std::map<std::string, Chips> &balances()
            const noexcept;

    private:
        std::map<std::string, Chips> accounts;
    };

}
