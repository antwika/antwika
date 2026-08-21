#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <vector>
#include <antwika/rng/IRng.hpp>
#include <antwika/voxel/VoxelCell.hpp>
#include <antwika/wfc/CompatibilityTable.hpp>
#include "antwika/worldgen/Axis.hpp"
#include "antwika/worldgen/ChunkShape.hpp"
#include "antwika/worldgen/Face.hpp"
#include "antwika/worldgen/Role.hpp"
#include "antwika/worldgen/RulesetFault.hpp"
#include "antwika/worldgen/Socket.hpp"
#include "antwika/worldgen/ruleset/District.hpp"
#include "antwika/worldgen/ruleset/Prototype.hpp"
#include "antwika/worldgen/ruleset/RulesetProblem.hpp"

namespace antwika::worldgen
{

    inline constexpr std::array<Face, kCubeFaces> kEveryFace{
        Face::East,
        Face::West,
        Face::Up,
        Face::Down,
        Face::North,
        Face::South};

    inline constexpr std::array<Axis, 3> kEveryAxis{
        Axis::Across, Axis::Upright, Axis::Along};

    [[nodiscard]] bool isDemand(Socket socket);

    [[nodiscard]] bool matesAcross(Socket oneSocket, Socket otherSocket);

    [[nodiscard]] bool matesUpright(Socket topSocket, Socket bottomSocket);

    inline constexpr std::size_t kEveryRoleCount = 6;

    inline constexpr std::array<Role, kEveryRoleCount> kEveryRole{
        Role::Room,
        Role::Perch,
        Role::Bear,
        Role::Climb,
        Role::Step,
        Role::Land};

    [[nodiscard]] std::uint8_t maskOf(Role role);

    struct Ruleset final
    {
        std::vector<Prototype> prototypes{};

        std::vector<District> districts{};

        [[nodiscard]] bool operator==(const Ruleset &other) const
            = default;
    };

    [[nodiscard]] std::vector<RulesetProblem> faultsIn(
        const Ruleset &ruleset);

}
