#pragma once

#include <cstddef>
#include <utility>
#include <vector>

#include <antwika/holdem/Action.hpp>
#include <antwika/holdem/IAgent.hpp>
#include <antwika/holdem/TableView.hpp>

namespace antwika::poker::tests
{

    class FakeAgent final : public holdem::IAgent
    {
    public:
        void script(std::vector<holdem::Action> actions)
        {
            this->actions = std::move(actions);
        }

        [[nodiscard]] holdem::Action act(
            const holdem::TableView &view) override
        {
            if (next == actions.size())
            {
                return view.toCall == 0 ? holdem::check() : holdem::fold();
            }
            return actions[next++];
        }

    private:
        std::vector<holdem::Action> actions;
        std::size_t next = 0;
    };

}
