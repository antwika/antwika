#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

#include <antwika/wfc/CompatibilityTable.hpp>

#include "antwika/worldgen/Axis.hpp"
#include "antwika/worldgen/ChunkShape.hpp"
#include "antwika/worldgen/Role.hpp"
#include "antwika/worldgen/ruleset/Prototype.hpp"

#include "antwika/worldgen/Ruleset.hpp"

namespace antwika::worldgen
{

    class CompiledRuleset final
    {
    public:
        explicit CompiledRuleset(Ruleset ruleset);

        [[nodiscard]] const Ruleset &source() const;

        [[nodiscard]] std::size_t size() const;

        [[nodiscard]] const Prototype &at(std::size_t which) const;

        [[nodiscard]] bool wears(std::size_t which, Role role) const;

        [[nodiscard]] std::span<const std::size_t> wearing(
            Role role) const;

        [[nodiscard]] std::span<const std::size_t> matching(
            voxel::Kind kind, voxel::Facing facing) const;

        [[nodiscard]] std::size_t districtOf(
            ChunkShape shape, std::int32_t level) const;

        [[nodiscard]] std::span<const std::uint32_t> desireIn(
            std::size_t district) const;

        [[nodiscard]] const wfc::CompatibilityTable &tableAlong(
            Axis axis) const;

        [[nodiscard]] std::shared_ptr<const wfc::CompatibilityTable>
        sharedTableAlong(Axis axis) const;

    private:
        Ruleset ruleset;

        std::array<std::shared_ptr<const wfc::CompatibilityTable>, 3>
            tables;

        std::array<std::vector<std::size_t>, kEveryRoleCount> byRole;

        std::vector<std::vector<std::size_t>> byKindAndFacing;
    };

}
