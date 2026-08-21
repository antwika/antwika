#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <set>

#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/voxel/VoxelDetail.hpp>
#include <antwika/voxel/VoxelStairs.hpp>

namespace antwika::voxel
{
    using namespace detail;

    namespace detail
    {

        [[nodiscard]] VoxelCell offsetBy(
            const VoxelCell fromCell, const VoxelCell byCell)
        {
            return VoxelCell{
                .x = fromCell.x + byCell.x,
                .y = fromCell.y + byCell.y,
                .z = fromCell.z + byCell.z};
        }

        constexpr std::array<VoxelCell, 4> kAboutCells{
            VoxelCell{.x = 1},
            VoxelCell{.x = -1},
            VoxelCell{.z = 1},
            VoxelCell{.z = -1}};

        constexpr VoxelCell kBelowCell{.y = -1};

        [[nodiscard]] VoxelCell rotated90(const VoxelCell stepCell)
        {
            return VoxelCell{.x = -stepCell.x, .y = -stepCell.y,
                             .z = -stepCell.z};
        }

        [[nodiscard]] std::optional<Kind> kindAt(
            const std::set<VoxelCell> &filledCells, const VoxelCell cell)
        {
            const auto foundCell = filledCells.find(cell);

            return foundCell == filledCells.end()
                              ? std::nullopt
                              : std::optional{foundCell->kind};
        }

        [[nodiscard]] std::optional<Kind> effectiveKindAt(
            const std::set<VoxelCell> &filledCells, VoxelCell cell);

        [[nodiscard]] std::optional<VoxelCell> voxelAt(
            const std::set<VoxelCell> &filledCells, const VoxelCell cell)
        {
            const auto foundCell = filledCells.find(cell);

            return foundCell == filledCells.end() ? std::nullopt
                              : std::optional{*foundCell};
        }

        [[nodiscard]] bool groundBeside(
            const std::set<VoxelCell> &filledCells,
            const VoxelCell cell,
            const VoxelCell stepCell)
        {
            const auto corner = cubeCornerOf(cell);

            for (std::int32_t upIndex = 0; upIndex < kCubeSide; ++upIndex)
            {
                for (std::int32_t alongIndex = 0; alongIndex < kCubeSide;
                     ++alongIndex)
                {
                    const auto besideCell = VoxelCell{
                        .x = corner.x
                             + (stepCell.x != 0
                                            ? (stepCell.x > 0 ? kCubeSide : -1)
                                            : alongIndex),
                        .y = corner.y + upIndex,
                        .z = corner.z
                             + (stepCell.z != 0
                                            ? (stepCell.z > 0 ? kCubeSide : -1)
                                            : alongIndex)};

                    if (kindAt(filledCells, besideCell) == Kind::Normal)
                    {
                        return true;
                    }
                }
            }

            return false;
        }

        [[nodiscard]] std::optional<VoxelCell> shapedClimb(
            const std::set<VoxelCell> &filledCells, const VoxelCell cell)
        {
            const auto corner = cubeCornerOf(cell);
            const auto top = corner.y + kCubeSide - 1;

            std::set<std::int32_t> acrossX;
            std::set<std::int32_t> acrossZ;

            for (std::int32_t offX = 0; offX < kCubeSide; ++offX)
            {
                for (std::int32_t offZ = 0; offZ < kCubeSide; ++offZ)
                {
                    const auto probeCell = VoxelCell{
                        .x = corner.x + offX,
                        .y = top,
                        .z = corner.z + offZ};

                    if (kindAt(filledCells, probeCell) == Kind::Ramp)
                    {
                        acrossX.insert(offX);
                        acrossZ.insert(offZ);
                    }
                }
            }

            if (acrossX.size() == 1 && acrossZ.size() > 1)
            {
                return VoxelCell{
                    .x = *acrossX.begin() == 0 ? -1 : 1};
            }

            if (acrossZ.size() == 1 && acrossX.size() > 1)
            {
                return VoxelCell{
                    .z = *acrossZ.begin() == 0 ? -1 : 1};
            }

            return std::nullopt;
        }

        [[nodiscard]] VoxelCell climbWithin(
            const std::set<VoxelCell> &filledCells, const VoxelCell cell)
        {
            if (const auto shapedCell = shapedClimb(filledCells, cell);
                shapedCell.has_value())
            {
                return *shapedCell;
            }

            const auto voxel = voxelAt(filledCells, cell);

            if (voxel.has_value() && voxel->facing != Facing::Any)
            {
                return stepVectorFor(voxel->facing);
            }

            for (const auto step : kAboutCells)
            {
                if (groundBeside(filledCells, cell, step)
                    && !groundBeside(
                        filledCells, cell, rotated90(step)))
                {
                    return step;
                }
            }

            for (const auto step : kAboutCells)
            {
                if (groundBeside(filledCells, cell, step))
                {
                    return step;
                }
            }

            const auto fills = [&filledCells](const VoxelCell place)
            { return effectiveKindAt(filledCells, place) == Kind::Normal; };

            for (const auto step : kAboutCells)
            {
                if (fills(offsetBy(cell, step))
                    && !fills(offsetBy(cell, rotated90(step))))
                {
                    return step;
                }
            }

            const auto aboveCell = offsetBy(cell, VoxelCell{.y = 1});

            if (kindAt(filledCells, aboveCell) == Kind::Ramp)
            {
                return climbWithin(filledCells, aboveCell);
            }

            for (const auto step : kAboutCells)
            {
                const auto open = offsetBy(cell, rotated90(step));

                if (!filledCells.contains(open)
                    && kindAt(filledCells, offsetBy(open, kBelowCell))
                           == Kind::Ramp)
                {
                    return step;
                }
            }

            return kAboutCells.front();
        }

        [[nodiscard]] StairHalf levelWithin(
            const std::set<VoxelCell> &filledCells, const VoxelCell cell)
        {
            if (cell.kind != Kind::Ramp)
            {
                return StairHalf::Any;
            }

            return kindAt(filledCells, offsetBy(cell, kBelowCell))
                           == Kind::Ramp
                            ? StairHalf::Upper
                            : StairHalf::Lower;
        }

        [[nodiscard]] bool isRampStep(
            const std::set<VoxelCell> &filledCells, const VoxelCell cell)
        {
            return cell.kind == Kind::Ramp
                   && !filledCells.contains(
                       offsetBy(cell, VoxelCell{.y = 1}));
        }

        [[nodiscard]] std::optional<Kind> effectiveKindAt(
            const std::set<VoxelCell> &filledCells, const VoxelCell cell)
        {
            const auto cellKind = kindAt(filledCells, cell);

            if (cellKind != Kind::Ramp)
            {
                return cellKind;
            }

            return isRampStep(
                       filledCells,
                       VoxelCell{
                           .x = cell.x,
                           .y = cell.y,
                           .z = cell.z,
                           .kind = Kind::Ramp})
                                 ? Kind::Ramp
                                 : Kind::Normal;
        }

    }

    VoxelCell inferredRampDirection(
        const std::vector<VoxelCell> &cells, const VoxelCell cell)
    {
        const std::set<VoxelCell> filledCells(cells.begin(), cells.end());

        return climbWithin(filledCells, cell);
    }

    VoxelCell inferredRampDirection(
        const std::set<VoxelCell> &filledCells, const VoxelCell cell)
    {
        return climbWithin(filledCells, cell);
    }

    Facing facingOfStep(const VoxelCell climbCell)
    {
        if (climbCell.x > 0)
        {
            return Facing::East;
        }

        if (climbCell.x < 0)
        {
            return Facing::West;
        }

        if (climbCell.z > 0)
        {
            return Facing::South;
        }

        return climbCell.z < 0 ? Facing::North : Facing::Any;
    }

    VoxelCell stepVectorFor(const Facing facing)
    {
        switch (facing)
        {
        case Facing::East:
            return VoxelCell{.x = 1};
        case Facing::West:
            return VoxelCell{.x = -1};
        case Facing::North:
            return VoxelCell{.z = -1};
        case Facing::South:
            return VoxelCell{.z = 1};
        case Facing::Any:
            break;
        }

        return VoxelCell{};
    }

    StairHalf stairHalfOf(
        const std::vector<VoxelCell> &cells, const VoxelCell cell)
    {
        const std::set<VoxelCell> filledCells(cells.begin(), cells.end());

        return levelWithin(filledCells, cell);
    }

    StairHalf stairHalfOf(
        const std::set<VoxelCell> &filledCells, const VoxelCell cell)
    {
        return levelWithin(filledCells, cell);
    }

}
