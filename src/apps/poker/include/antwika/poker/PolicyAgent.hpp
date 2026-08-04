#pragma once

#include <array>

#include <antwika/holdem/HandCategory.hpp>
#include <antwika/holdem/Action.hpp>
#include <antwika/holdem/IAgent.hpp>
#include <antwika/holdem/TableView.hpp>

#include "antwika/poker/AgentStyle.hpp"

namespace antwika::poker
{

    using antwika::holdem::Action;
    using antwika::holdem::kHandCategoryCount;
    using antwika::holdem::IAgent;
    using antwika::holdem::TableView;

    /**
     * @brief An agent that plays by a fixed, deterministic policy.
     *
     * Two properties matter more here than playing well. It is a pure
     * function of the view it is handed, so a replayed session reaches
     * the same decisions without any of them having to be recorded; and
     * it never returns an action Table would reject, because every
     * wager it names is clamped into the legal range rather than
     * offered and hoped for.
     *
     * The policy itself is deliberately plain -- a strength score
     * compared against two thresholds -- and makes no attempt to read
     * opponents or to notice that a pair on the board is shared by
     * everybody.
     */
    class PolicyAgent final : public IAgent
    {
    public:
        /**
         * @brief Construct the agent with a playing style.
         * @param style How willing it should be to commit chips.
         */
        /**
         * @brief Construct an agent over its style and its ratings.
         * @param style How boldly this agent plays.
         * @param handStrengths How strongly each made hand is rated,
         * weakest category first; RoomConfig carries the table the
         * config file states.
         */
        PolicyAgent(
            AgentStyle style,
            std::array<unsigned, kHandCategoryCount>
                handStrengths) noexcept;

        /**
         * @brief Decide what to do with the hand in front of it.
         * @param view Everything this player is entitled to know.
         * @return A legal action for that view.
         */
        [[nodiscard]] Action act(const TableView &view) override;

        /**
         * @brief Read this agent's style.
         * @return The style it was constructed with.
         */
        [[nodiscard]] AgentStyle playingStyle() const noexcept;

    private:
        AgentStyle style;
        std::array<unsigned, kHandCategoryCount> handStrengths;
    };

    /**
     * @brief Score how strong a view's hand is, from 0 to 100.
     *
     * Exposed because it is the whole of the agent's judgement and
     * deserves testing on its own: pre-flop it reads the two hole cards,
     * and from the flop on it asks HandEvaluator what they make with the
     * board.
     * @param view The hand to judge.
     * @return A strength score in [0, 100].
     */
    [[nodiscard]] unsigned handStrength(
        const TableView &view,
        const std::array<unsigned, kHandCategoryCount> &handStrengths);

} // namespace antwika::poker
