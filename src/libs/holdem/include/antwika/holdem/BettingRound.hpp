#pragma once

#include "antwika/holdem/Chips.hpp"
#include "antwika/holdem/TableMemory.hpp"

namespace antwika::holdem
{

    class BettingRound final
    {
    public:
        void open(Chips bigBlind) noexcept;

        void reset(Chips bigBlind) noexcept;

        void close() noexcept;

        [[nodiscard]] Chips bet() const noexcept;

        [[nodiscard]] bool isLive() const noexcept;

        [[nodiscard]] Chips minimumRaiseTo() const noexcept;

        [[nodiscard]] Chips owedBy(Chips roundCommitted) const noexcept;

        [[nodiscard]] bool isCovered(Chips roundCommitted) const noexcept;

        [[nodiscard]] bool raiseTo(Chips target, Chips allInTo);

        [[nodiscard]] BettingMemory remember() const noexcept;

        void restore(const BettingMemory &memory) noexcept;

    private:
        Chips currentBet = 0;
        Chips lastRaiseSize = 0;
    };

}
