#include "antwika/worldgen/Ruleset.hpp"

#include <algorithm>
#include <utility>

#include "antwika/worldgen/WorldgenError.hpp"
#include "antwika/worldgen/ruleset/CompiledRuleset.hpp"

namespace antwika::worldgen
{

    namespace
    {
        struct Pair final
        {
            Socket oneSocket;
            Socket otherSocket;
        };

        constexpr std::array kAcrossPairs{
            Pair{Socket::OpenSide, Socket::OpenSide},
            Pair{Socket::OpenSide, Socket::RoomSide},
            Pair{Socket::RoomSide, Socket::RoomSide},
            Pair{Socket::OpenSide, Socket::Facade},
            Pair{Socket::RoomSide, Socket::Facade},
            Pair{Socket::OpenSide, Socket::LandingSide},
            Pair{Socket::RoomSide, Socket::LandingSide},
            Pair{Socket::Facade, Socket::Facade},
            Pair{Socket::Facade, Socket::LandingSide},
            Pair{Socket::Facade, Socket::Buried},
            Pair{Socket::LandingSide, Socket::LandingSide},
            Pair{Socket::LandingSide, Socket::Buried},
            Pair{Socket::Buried, Socket::Buried},
            Pair{Socket::WaterSide, Socket::WaterSide},
            Pair{Socket::WaterSide, Socket::Facade},
            Pair{Socket::WaterSide, Socket::LandingSide},
            Pair{Socket::WaterSide, Socket::Buried},
            Pair{Socket::StairSideEast, Socket::StairSideEast},
            Pair{Socket::StairSideEast, Socket::OpenSide},
            Pair{Socket::StairSideEast, Socket::RoomSide},
            Pair{Socket::StairSideEast, Socket::Facade},
            Pair{Socket::StairSideEast, Socket::LandingSide},
            Pair{Socket::StairSideWest, Socket::StairSideWest},
            Pair{Socket::StairSideWest, Socket::OpenSide},
            Pair{Socket::StairSideWest, Socket::RoomSide},
            Pair{Socket::StairSideWest, Socket::Facade},
            Pair{Socket::StairSideWest, Socket::LandingSide},
            Pair{Socket::StairSideNorth, Socket::StairSideNorth},
            Pair{Socket::StairSideNorth, Socket::OpenSide},
            Pair{Socket::StairSideNorth, Socket::RoomSide},
            Pair{Socket::StairSideNorth, Socket::Facade},
            Pair{Socket::StairSideNorth, Socket::LandingSide},
            Pair{Socket::StairSideSouth, Socket::StairSideSouth},
            Pair{Socket::StairSideSouth, Socket::OpenSide},
            Pair{Socket::StairSideSouth, Socket::RoomSide},
            Pair{Socket::StairSideSouth, Socket::Facade},
            Pair{Socket::StairSideSouth, Socket::LandingSide},
            Pair{Socket::NeedsRoot, Socket::Facade},
            Pair{Socket::NeedsRoot, Socket::LandingSide},
            Pair{Socket::NeedsRoot, Socket::Buried},
            Pair{Socket::NeedsLanding, Socket::LandingSide},
            Pair{Socket::NeedsApproach, Socket::RoomSide},
            Pair{Socket::NeedsOpen, Socket::OpenSide},
            Pair{Socket::NeedsOpen, Socket::RoomSide},
            Pair{Socket::NeedsBack, Socket::Facade},
            Pair{Socket::NeedsBack, Socket::LandingSide},
            Pair{Socket::NeedsBack, Socket::Buried}};

        constexpr std::array kUprightPairs{
            Pair{Socket::Sky, Socket::Floats},
            Pair{Socket::Sky, Socket::Hangs},
            Pair{Socket::Carries, Socket::Rests},
            Pair{Socket::Carries, Socket::Stands},
            Pair{Socket::Carries, Socket::Climbs},
            Pair{Socket::Carries, Socket::Grips},
            Pair{Socket::Carries, Socket::Submerged},
            Pair{Socket::Carries, Socket::Rooted},
            Pair{Socket::Terrace, Socket::Stands},
            Pair{Socket::Terrace, Socket::Climbs},
            Pair{Socket::Terrace, Socket::Grips},
            Pair{Socket::StairHead, Socket::Stands},
            Pair{Socket::Rung, Socket::Grips},
            Pair{Socket::Rung, Socket::Stands},
            Pair{Socket::WaterTop, Socket::Submerged},
            Pair{Socket::WaterTop, Socket::Stands},
            Pair{Socket::WaterTop, Socket::Rests},
            Pair{Socket::WaterTop, Socket::Climbs},
            Pair{Socket::WaterTop, Socket::Grips}};

        constexpr std::array kDemands{
            Socket::NeedsRoot,
            Socket::NeedsLanding,
            Socket::NeedsApproach,
            Socket::NeedsOpen,
            Socket::NeedsBack};

        [[nodiscard]] Face getOpposing(const Face face)
        {
            switch (face)
            {
            case Face::East:
                return Face::West;
            case Face::West:
                return Face::East;
            case Face::Up:
                return Face::Down;
            case Face::Down:
                return Face::Up;
            case Face::North:
                return Face::South;
            case Face::South:
                break;
            }

            return Face::North;
        }

        [[nodiscard]] Socket getShows(const Prototype &prototype, const Face face)
        {
            return prototype.sockets[static_cast<std::size_t>(face)];
        }

        [[nodiscard]] bool meetsSomething(
            const std::vector<Prototype> &prototypes,
            const Prototype &prototype,
            const Face face)
        {
            const Socket mineSocket = getShows(prototype, face);
            const Face againstFace = getOpposing(face);

            for (const Prototype &other : prototypes)
            {
                const Socket theirsSocket = getShows(other, againstFace);

                if (face == Face::Up && matesUpright(mineSocket, theirsSocket))
                {
                    return true;
                }

                if (face == Face::Down
                    && matesUpright(theirsSocket, mineSocket))
                {
                    return true;
                }

                if (face != Face::Up && face != Face::Down
                    && matesAcross(mineSocket, theirsSocket))
                {
                    return true;
                }
            }

            return false;
        }

        [[nodiscard]] std::size_t keyOf(
            const voxel::Kind kind, const voxel::Facing facing)
        {
            return (static_cast<std::size_t>(kind) * voxel::kEveryFacing.size())
                   + static_cast<std::size_t>(facing);
        }
    }

    bool isDemand(const Socket socket)
    {
        return std::ranges::find(kDemands, socket) != kDemands.end();
    }

    bool matesAcross(const Socket oneSocket, const Socket otherSocket)
    {
        for (const Pair pair : kAcrossPairs)
        {
            if (pair.oneSocket == oneSocket && pair.otherSocket == otherSocket)
            {
                return true;
            }

            if (pair.oneSocket == otherSocket && pair.otherSocket == oneSocket)
            {
                return true;
            }
        }

        return false;
    }

    bool matesUpright(const Socket topSocket, const Socket bottomSocket)
    {
        for (const Pair pair : kUprightPairs)
        {
            if (pair.oneSocket == topSocket && pair.otherSocket == bottomSocket)
            {
                return true;
            }
        }

        return false;
    }

    std::uint8_t maskOf(const Role role)
    {
        return static_cast<std::uint8_t>(
            1U << static_cast<unsigned>(role));
    }

    std::vector<RulesetProblem> faultsIn(const Ruleset &ruleset)
    {
        std::vector<RulesetProblem> problems;

        if (ruleset.prototypes.empty())
        {
            problems.push_back(
                RulesetProblem{.fault = RulesetFault::NoPrototypes});
            return problems;
        }

        for (std::size_t index = 0; index < ruleset.prototypes.size(); ++index)
        {
            const Prototype &prototype = ruleset.prototypes[index];

            if (prototype.kind == voxel::Kind::Ramp && !prototype.air
                && prototype.facing == voxel::Facing::Any)
            {
                problems.push_back(
                    RulesetProblem{
                        .fault = RulesetFault::RampWithoutFacing,
                        .prototypeIndex = index});
            }

            for (const Face face : kEveryFace)
            {
                if (!meetsSomething(ruleset.prototypes, prototype, face))
                {
                    problems.push_back(
                        RulesetProblem{
                            .fault = RulesetFault::FaceMeetsNothing,
                            .prototypeIndex = index,
                            .face = face});
                }
            }
        }

        for (const Role role : kEveryRole)
        {
            const auto worn = [&](const Prototype &prototype)
            { return (prototype.roles & maskOf(role)) != 0; };

            if (std::ranges::none_of(ruleset.prototypes, worn))
            {
                problems.push_back(
                    RulesetProblem{
                        .fault = RulesetFault::RoleWornByNoPrototype,
                        .prototypeIndex = static_cast<std::size_t>(role)});
            }
        }

        if (ruleset.districts.empty())
        {
            problems.push_back(
                RulesetProblem{.fault = RulesetFault::NoDistricts});
            return problems;
        }

        std::uint8_t reachedRoles = 0;
        for (std::size_t index = 0; index < ruleset.districts.size(); ++index)
        {
            const District &district = ruleset.districts[index];

            if (district.untilShare <= reachedRoles
                || (index + 1 == ruleset.districts.size()
                    && district.untilShare != 100))
            {
                problems.push_back(
                    RulesetProblem{
                        .fault = RulesetFault::DistrictsDoNotRise,
                        .prototypeIndex = index});
            }
            reachedRoles = district.untilShare;

            if (district.desire.size() != ruleset.prototypes.size())
            {
                problems.push_back(
                    RulesetProblem{
                        .fault = RulesetFault::DistrictMissizes,
                        .prototypeIndex = index});
                continue;
            }

            const auto wantsAny = [](const std::uint32_t much)
            { return much != 0; };

            if (std::ranges::none_of(district.desire, wantsAny))
            {
                problems.push_back(
                    RulesetProblem{
                        .fault = RulesetFault::DistrictAllowsNothing,
                        .prototypeIndex = index});
            }
        }

        return problems;
    }

    CompiledRuleset::CompiledRuleset(Ruleset ruleset)
        : ruleset(std::move(ruleset))
    {
        if (!faultsIn(this->ruleset).empty())
        {
            throw WorldgenError(
                "CompiledRuleset: the ruleset has faults in it");
        }

        const std::size_t count = this->ruleset.prototypes.size();

        for (const Axis axis : kEveryAxis)
        {
            wfc::CompatibilityTable table(count);

            for (std::size_t lowIndex = 0; lowIndex < count; ++lowIndex)
            {
                for (std::size_t highIndex = 0; highIndex < count; ++highIndex)
                {
                    const Prototype &underPrototype =
                        this->ruleset.prototypes[lowIndex];
                    const Prototype &overPrototype =
                        this->ruleset.prototypes[highIndex];

                    const bool fits =
                        axis == Axis::Across
                              ? matesAcross(
                                  getShows(underPrototype, Face::East),
                                  getShows(overPrototype, Face::West))
                        : axis == Axis::Upright
                            ? matesUpright(
                                  getShows(underPrototype, Face::Up),
                                  getShows(overPrototype, Face::Down))
                            : matesAcross(
                                  getShows(underPrototype, Face::South),
                                  getShows(overPrototype, Face::North));

                    table.set(lowIndex, highIndex, fits);
                }
            }

            tables[static_cast<std::size_t>(axis)] =
                std::make_shared<const wfc::CompatibilityTable>(
                    std::move(table));
        }

        for (const Role role : kEveryRole)
        {
            for (std::size_t index = 0; index < count; ++index)
            {
                if ((this->ruleset.prototypes[index].roles & maskOf(role)) != 0)
                {
                    byRole[static_cast<std::size_t>(role)].push_back(index);
                }
            }
        }

        byKindAndFacing.assign(
            voxel::kEveryKind.size() * voxel::kEveryFacing.size(), {});

        for (std::size_t index = 0; index < count; ++index)
        {
            const Prototype &prototype = this->ruleset.prototypes[index];

            if (prototype.air)
            {
                continue;
            }

            for (const voxel::Facing facing : voxel::kEveryFacing)
            {
                const bool serves = facing == voxel::Facing::Any
                                    || prototype.facing == voxel::Facing::Any
                                    || prototype.facing == facing;

                if (serves)
                {
                    byKindAndFacing[keyOf(prototype.kind, facing)]
                        .push_back(index);
                }
            }
        }
    }

    const Ruleset &CompiledRuleset::getSource() const
    {
        return ruleset;
    }

    std::size_t CompiledRuleset::getSize() const
    {
        return ruleset.prototypes.size();
    }

    const Prototype &CompiledRuleset::getEntryAt(const std::size_t which) const
    {
        if (which >= ruleset.prototypes.size())
        {
            throw WorldgenError("CompiledRuleset: there is no such prototype");
        }

        return ruleset.prototypes[which];
    }

    bool CompiledRuleset::wears(
        const std::size_t which, const Role role) const
    {
        return (getEntryAt(which).roles & maskOf(role)) != 0;
    }

    std::span<const std::size_t> CompiledRuleset::getWearing(
        const Role role) const
    {
        return byRole[static_cast<std::size_t>(role)];
    }

    std::span<const std::size_t> CompiledRuleset::getMatching(
        const voxel::Kind kind, const voxel::Facing facing) const
    {
        return byKindAndFacing[keyOf(kind, facing)];
    }

    std::size_t CompiledRuleset::districtOf(
        const ChunkShape shape, const std::int32_t level) const
    {
        if (shape.height <= 0)
        {
            throw WorldgenError("districtOf: the chunk has no height");
        }

        const std::int32_t clampedLevel =
            std::clamp(level, 0, shape.height - 1);
        const auto share =
            static_cast<std::uint8_t>((clampedLevel * 100) / shape.height);

        for (std::size_t index = 0; index < ruleset.districts.size(); ++index)
        {
            if (share < ruleset.districts[index].untilShare)
            {
                return index;
            }
        }

        return ruleset.districts.size() - 1; // GCOVR_EXCL_LINE
    }

    std::span<const std::uint32_t> CompiledRuleset::desireIn(
        const std::size_t district) const
    {
        if (district >= ruleset.districts.size())
        {
            throw WorldgenError("desireIn: there is no such district");
        }

        return ruleset.districts[district].desire;
    }

    const wfc::CompatibilityTable &CompiledRuleset::tableAlong(
        const Axis axis) const
    {
        return *tables[static_cast<std::size_t>(axis)];
    }

    std::shared_ptr<const wfc::CompatibilityTable>
    CompiledRuleset::sharedTableAlong(const Axis axis) const
    {
        return tables[static_cast<std::size_t>(axis)];
    }

}
