#include "antwika/worldgen/CityRuleset.hpp"

#include <initializer_list>
#include <utility>

namespace antwika::worldgen
{

    namespace
    {
        using voxel::Facing;
        using voxel::Kind;

        [[nodiscard]] std::uint8_t rolesOf(
            const std::initializer_list<Role> roles)
        {
            std::uint8_t gatheredCount = 0;

            for (const Role role : roles)
            {
                gatheredCount =
                static_cast<std::uint8_t>(gatheredCount | maskOf(role));
            }

            return gatheredCount;
        }

        [[nodiscard]] Prototype getAir(
            const char *name,
            const Socket sideSocket,
            const Socket bottomSocket)
        {
            return Prototype{
                .name = name,
                .kind = Kind::Normal,
                .facing = Facing::Any,
                .air = true,
                .sockets =
                    {sideSocket,
                     sideSocket,
                     Socket::Sky,
                     bottomSocket,
                     sideSocket,
                     sideSocket},
                .roles = bottomSocket == Socket::Stands
                       ? rolesOf({Role::Room, Role::Perch})
                       : std::uint8_t{0}};
        }

        [[nodiscard]] Prototype getBlock(
            const char *name,
            const Socket sideSocket,
            const Socket topSocket,
            const Socket bottomSocket,
            const std::uint8_t roles)
        {
            return Prototype{
                .name = name,
                .kind = Kind::Normal,
                .facing = Facing::Any,
                .air = false,
                .sockets =
                    {sideSocket,
                     sideSocket,
                     topSocket,
                     bottomSocket,
                     sideSocket,
                     sideSocket},
                .roles = roles};
        }

        [[nodiscard]] Prototype getCorbel(const char *name, const Face rootFace)
        {
            Prototype madePrototype = getBlock(
                name,
                Socket::Facade,
                Socket::Carries,
                Socket::Hangs,
                rolesOf({Role::Bear}));

            madePrototype.sockets[static_cast<std::size_t>(rootFace)] =
                Socket::NeedsRoot;

            return madePrototype;
        }

        [[nodiscard]] Prototype getStair(
            const char *name,
            const Facing facing,
            const Face headFace,
            const Face footFace,
            const Socket flankSocket)
        {
            Prototype madePrototype{
                .name = name,
                .kind = Kind::Ramp,
                .facing = facing,
                .air = false,
                .sockets =
                    {flankSocket,
                     flankSocket,
                     Socket::StairHead,
                     Socket::Climbs,
                     flankSocket,
                     flankSocket},
                .roles = rolesOf({Role::Step})};

            madePrototype.sockets[static_cast<std::size_t>(headFace)] =
                Socket::NeedsLanding;
            madePrototype.sockets[static_cast<std::size_t>(footFace)] =
                Socket::NeedsApproach;

            return madePrototype;
        }

        [[nodiscard]] District getQuarter(
            const char *name,
            const std::uint8_t untilShare,
            const std::initializer_list<
                std::pair<CityPiece, std::uint32_t>> wants)
        {
            District madeDistrict{
                .name = name,
                .untilShare = untilShare,
                .desire = std::vector<std::uint32_t>(kCityPieces, 0)};

            for (const auto &[piece, much] : wants)
            {
                madeDistrict.desire[indexOf(piece)] = much;
            }

            return madeDistrict;
        }
    }

    std::size_t indexOf(const CityPiece piece)
    {
        return static_cast<std::size_t>(piece);
    }

    Ruleset getCityRuleset()
    {
        Ruleset ruleset;

        ruleset.prototypes = {
            getAir("air open", Socket::OpenSide, Socket::Floats),
            getAir("air room", Socket::RoomSide, Socket::Stands),
            getBlock(
                "bedrock",
                Socket::Facade,
                Socket::Carries,
                Socket::Rooted,
                rolesOf({Role::Bear})),
            getBlock(
                "fill",
                Socket::Buried,
                Socket::Carries,
                Socket::Rests,
                rolesOf({Role::Bear})),
            getBlock(
                "wall",
                Socket::Facade,
                Socket::Carries,
                Socket::Rests,
                rolesOf({Role::Bear})),
            getBlock(
                "floor",
                Socket::LandingSide,
                Socket::Terrace,
                Socket::Rests,
                rolesOf({Role::Bear, Role::Land})),
            getCorbel("corbel east", Face::East),
            getCorbel("corbel west", Face::West),
            getCorbel("corbel north", Face::North),
            getCorbel("corbel south", Face::South),
            getStair(
                "stair east",
                Facing::East,
                Face::East,
                Face::West,
                Socket::StairSideEast),
            getStair(
                "stair west",
                Facing::West,
                Face::West,
                Face::East,
                Socket::StairSideWest),
            getStair(
                "stair north",
                Facing::North,
                Face::North,
                Face::South,
                Socket::StairSideNorth),
            getStair(
                "stair south",
                Facing::South,
                Face::South,
                Face::North,
                Socket::StairSideSouth),
            getBlock(
                "cistern",
                Socket::WaterSide,
                Socket::WaterTop,
                Socket::Submerged,
                0)};

        ruleset.prototypes[indexOf(CityPiece::Cistern)].kind = Kind::Water;

        ruleset.districts = {
            getQuarter(
                "bedrock",
                5,
                {{CityPiece::Bedrock, 40},
                 {CityPiece::AirOpen, 3},
                 {CityPiece::AirRoom, 5},
                 {CityPiece::Fill, 10},
                 {CityPiece::Wall, 8},
                 {CityPiece::Floor, 4},
                 {CityPiece::Cistern, 2},
                 {CityPiece::StairEast, 1},
                 {CityPiece::StairWest, 1},
                 {CityPiece::StairNorth, 1},
                 {CityPiece::StairSouth, 1}}),
            getQuarter(
                "undercroft",
                16,
                {{CityPiece::AirOpen, 6},
                 {CityPiece::AirRoom, 14},
                 {CityPiece::Fill, 10},
                 {CityPiece::Wall, 14},
                 {CityPiece::Floor, 8},
                 {CityPiece::Cistern, 4},
                 {CityPiece::StairEast, 2},
                 {CityPiece::StairWest, 2},
                 {CityPiece::StairNorth, 2},
                 {CityPiece::StairSouth, 2}}),
            getQuarter(
                "slums",
                40,
                {{CityPiece::AirOpen, 18},
                 {CityPiece::AirRoom, 16},
                 {CityPiece::Fill, 8},
                 {CityPiece::Wall, 16},
                 {CityPiece::Floor, 10},
                 {CityPiece::CorbelEast, 3},
                 {CityPiece::CorbelWest, 3},
                 {CityPiece::CorbelNorth, 3},
                 {CityPiece::CorbelSouth, 3},
                 {CityPiece::StairEast, 4},
                 {CityPiece::StairWest, 4},
                 {CityPiece::StairNorth, 4},
                 {CityPiece::StairSouth, 4}}),
            getQuarter(
                "middling",
                68,
                {{CityPiece::AirOpen, 30},
                 {CityPiece::AirRoom, 14},
                 {CityPiece::Fill, 4},
                 {CityPiece::Wall, 12},
                 {CityPiece::Floor, 12},
                 {CityPiece::CorbelEast, 3},
                 {CityPiece::CorbelWest, 3},
                 {CityPiece::CorbelNorth, 3},
                 {CityPiece::CorbelSouth, 3},
                 {CityPiece::StairEast, 5},
                 {CityPiece::StairWest, 5},
                 {CityPiece::StairNorth, 5},
                 {CityPiece::StairSouth, 5}}),
            getQuarter(
                "heights",
                90,
                {{CityPiece::AirOpen, 52},
                 {CityPiece::AirRoom, 10},
                 {CityPiece::Wall, 7},
                 {CityPiece::Floor, 8},
                 {CityPiece::CorbelEast, 2},
                 {CityPiece::CorbelWest, 2},
                 {CityPiece::CorbelNorth, 2},
                 {CityPiece::CorbelSouth, 2},
                 {CityPiece::StairEast, 3},
                 {CityPiece::StairWest, 3},
                 {CityPiece::StairNorth, 3},
                 {CityPiece::StairSouth, 3}}),
            getQuarter(
                "sky",
                100,
                {{CityPiece::AirOpen, 60}, {CityPiece::AirRoom, 2}})};

        return ruleset;
    }

}
