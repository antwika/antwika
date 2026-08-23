#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <vector>

#include "antwika/wfc/Domain.hpp"
#include "antwika/wfc/IConstraint.hpp"
#include "antwika/wfc/SolveResult.hpp"
#include "antwika/wfc/SolverLimits.hpp"

namespace antwika::wfc
{

    class Solver final
    {
    public:
        Solver(
            std::vector<Domain> initialWaveDomains,
            std::vector<std::reference_wrapper<const IConstraint>>
                constraints,
            std::vector<double> valueWeights = {},
            SolverLimits limits = {},
            std::vector<std::optional<std::size_t>> preferences = {});

        [[nodiscard]] SolveResult getSolve() const;

    private:
        std::vector<Domain> initialWave;
        std::vector<std::reference_wrapper<const IConstraint>> constraints;
        std::vector<double> valueWeights;
        SolverLimits limits;
        std::vector<std::optional<std::size_t>> preferences;

        std::vector<std::vector<std::size_t>> cellToConstraints;
    };

}
