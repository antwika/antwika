#pragma once

#include "antwika/holdem/Action.hpp"
#include "antwika/holdem/TableView.hpp"

namespace antwika::holdem
{

    class IAgent
    {
    public:
        virtual ~IAgent() = default;

        [[nodiscard]] virtual Action act(const TableView &view) = 0;
    };

}
