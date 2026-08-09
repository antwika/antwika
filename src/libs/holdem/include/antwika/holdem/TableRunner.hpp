#pragma once

#include <functional>
#include <vector>

#include "antwika/holdem/IAgent.hpp"
#include "antwika/holdem/IDeck.hpp"
#include "antwika/holdem/StepOutcome.hpp"
#include "antwika/holdem/Table.hpp"

namespace antwika::holdem
{

    class TableRunner final
    {
    public:
        TableRunner(
            Table &table,
            IDeck &deck,
            std::vector<std::reference_wrapper<IAgent>> agents);

        TableRunner(const TableRunner &) = delete;
        TableRunner(TableRunner &&) = delete;

        TableRunner &operator=(const TableRunner &) = delete;
        TableRunner &operator=(TableRunner &&) = delete;

        StepOutcome step();

    private:
        Table &table;
        IDeck &deck;
        std::vector<std::reference_wrapper<IAgent>> agents;
    };

}
