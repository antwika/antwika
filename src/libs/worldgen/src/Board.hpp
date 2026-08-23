#pragma once

#include <cstddef>
#include <span>
#include <vector>
#include <antwika/wfc/Domain.hpp>
#include <antwika/worldgen/ruleset/CompiledRuleset.hpp>
#include "antwika/worldgen/ChunkShape.hpp"
#include "antwika/worldgen/Ruleset.hpp"

namespace antwika::worldgen::detail
{

    class Board final
    {
    public:
        explicit Board(std::vector<wfc::Domain> &waveDomains);

        [[nodiscard]] const std::vector<wfc::Domain> &getWave() const;

        [[nodiscard]] bool holds(
            std::size_t cell, std::size_t value) const;

        void take(std::size_t cell, std::size_t value);

        void hold(std::size_t cell, std::span<const std::size_t> wantedCells);

        [[nodiscard]] std::size_t getMarkStep() const;

        void rewind(std::size_t toStep);

    private:
        struct Collapse final
        {
            std::size_t cell;
            std::size_t value;
        };

        std::vector<wfc::Domain> *heldDomains;

        std::vector<Collapse> trailCollapses{};
    };

}
