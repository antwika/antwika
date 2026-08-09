#pragma once

#include <array>

#include <antwika/holdem/HandCategory.hpp>
#include <antwika/holdem/Action.hpp>
#include <antwika/holdem/IAgent.hpp>
#include <antwika/holdem/TableView.hpp>

#include "antwika/poker/AgentStyle.hpp"
#include "antwika/poker/RoomConfig.hpp"

namespace antwika::poker
{

    using antwika::holdem::Action;
    using antwika::holdem::kHandCategoryCount;
    using antwika::holdem::IAgent;
    using antwika::holdem::TableView;

    class PolicyAgent final : public IAgent
    {
    public:
        PolicyAgent(
            AgentStyle style,
            std::array<unsigned, kHandCategoryCount> handStrengths,
            std::array<AgentThresholds, kAgentStyleCount>
                thresholds) noexcept;

        [[nodiscard]] Action act(const TableView &view) override;

        [[nodiscard]] AgentStyle playingStyle() const noexcept;

    private:
        AgentStyle style;
        std::array<unsigned, kHandCategoryCount> handStrengths;
        std::array<AgentThresholds, kAgentStyleCount> thresholds;
    };

    [[nodiscard]] unsigned handStrength(
        const TableView &view,
        const std::array<unsigned, kHandCategoryCount> &handStrengths);

}
