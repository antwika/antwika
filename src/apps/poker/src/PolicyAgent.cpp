#include "antwika/poker/PolicyAgent.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <vector>

#include <antwika/holdem/Action.hpp>
#include <antwika/holdem/Card.hpp>
#include <antwika/holdem/Chips.hpp>
#include <antwika/holdem/HandCategory.hpp>
#include <antwika/holdem/HandEvaluator.hpp>
#include <antwika/holdem/HandValue.hpp>
#include <antwika/holdem/Limits.hpp>

namespace antwika::poker
{

    using antwika::holdem::bet;
    using antwika::holdem::call;
    using antwika::holdem::Card;
    using antwika::holdem::categoryOf;
    using antwika::holdem::check;
    using antwika::holdem::Chips;
    using antwika::holdem::evaluate;
    using antwika::holdem::fold;
    using antwika::holdem::HandCategory;
    using antwika::holdem::raiseTo;
    using antwika::holdem::rankOf;
    using antwika::holdem::rawValue;
    using antwika::holdem::suitOf;

    namespace
    {

        struct Thresholds
        {
            unsigned raiseAt = 0;
            unsigned callAt = 0;
        };

        [[nodiscard]] Thresholds thresholdsFor(AgentStyle style) noexcept
        {
            if (style == AgentStyle::Tight)
            {
                return Thresholds{.raiseAt = 80, .callAt = 55};
            }
            if (style == AgentStyle::Aggressive)
            {
                return Thresholds{.raiseAt = 55, .callAt = 25};
            }
            return Thresholds{.raiseAt = 70, .callAt = 40};
        }

        [[nodiscard]] unsigned preFlopStrength(
            const std::array<Card, antwika::holdem::kHoleCardCount> &hole)
        {
            const auto first = rawValue(rankOf(hole[0]));
            const auto second = rawValue(rankOf(hole[1]));
            const auto high = std::max(first, second);
            const auto low = std::min(first, second);

            unsigned strength = 10U + high * 4U;
            if (first == second)
            {
                strength += 30U;
            }
            else
            {
                if (suitOf(hole[0]) == suitOf(hole[1]))
                {
                    strength += 8U;
                }
                const auto gap = high - low;
                if (gap == 1)
                {
                    strength += 6U;
                }
                else if (gap >= 4)
                {
                    strength -= 8U;
                }
            }
            return std::min(strength, 100U);
        }

        // Indexed by HandCategory, weakest first.
        // A table rather than a switch, with no unreachable default.
        constexpr std::array<unsigned, 9> kStrengthByCategory{
            20,
            45,
            62,
            76,
            85,
            90,
            95,
            98,
            100,
        };

        [[nodiscard]] unsigned madeHandStrength(
            HandCategory category) noexcept
        {
            return kStrengthByCategory[static_cast<std::size_t>(category)];
        }

        // Sizes a wager at roughly the pot, then clamps it.
        // That is what keeps every action this agent names legal.
        [[nodiscard]] Chips wagerTarget(const TableView &view) noexcept
        {
            const auto potSized = view.currentBet + view.pot;
            const auto target = std::max(view.minRaiseTo, potSized);
            return std::min(target, view.maxRaiseTo);
        }

    } // namespace

    PolicyAgent::PolicyAgent(AgentStyle style) noexcept : style(style)
    {
    }

    AgentStyle PolicyAgent::playingStyle() const noexcept
    {
        return style;
    }

    Action PolicyAgent::act(const TableView &view)
    {
        const auto strength = handStrength(view);
        const auto thresholds = thresholdsFor(style);
        const auto canRaise =
            view.mayRaise && view.maxRaiseTo > view.currentBet;

        if (strength >= thresholds.raiseAt && canRaise)
        {
            const auto target = wagerTarget(view);
            return view.currentBet == 0 ? bet(target) : raiseTo(target);
        }

        if (view.toCall == 0)
        {
            return check();
        }
        if (strength >= thresholds.callAt)
        {
            return call();
        }
        return fold();
    }

    unsigned handStrength(const TableView &view)
    {
        if (view.board.empty())
        {
            return preFlopStrength(view.holeCards);
        }

        std::vector<Card> cards(
            view.holeCards.begin(), view.holeCards.end());
        cards.insert(cards.end(), view.board.begin(), view.board.end());
        return madeHandStrength(categoryOf(evaluate(cards)));
    }

} // namespace antwika::poker
