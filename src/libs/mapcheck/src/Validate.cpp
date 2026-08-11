#include "antwika/mapcheck/Validate.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

#include <antwika/tilemap/Column.hpp>
#include <antwika/tilemap/Entities.hpp>
#include <antwika/tilemap/FlowDirection.hpp>
#include <antwika/tilemap/Overlay.hpp>
#include <antwika/tilemap/Slab.hpp>
#include <antwika/tilemap/TerrainClass.hpp>

namespace antwika::mapcheck
{

    namespace
    {
        using geometry::GridCell;
        using tilemap::Column;
        using tilemap::Entity;
        using tilemap::FlowDirection;
        using tilemap::Overlay;
        using tilemap::Pickup;
        using tilemap::Slab;
        using tilemap::TerrainClass;
        using tilemap::TileMap;
        using tilemap::Transition;
        using tilemap::TriggerVolume;

        constexpr std::string_view kSwimTag = "swim";

        constexpr std::array kDirections = {
            FlowDirection::North,
            FlowDirection::East,
            FlowDirection::South,
            FlowDirection::West,
        };

        struct Surface final
        {
            GridCell cell{};
            std::size_t slab = 0;

            [[nodiscard]] bool operator==( // GCOVR_EXCL_LINE
                const Surface &other) const = default;
        };

        [[nodiscard]] bool inBounds(const TileMap &map, const GridCell cell)
        {
            return cell.column < map.columns() && cell.row < map.rows();
        }

        [[nodiscard]] std::size_t cellIndexOf(
            const TileMap &map, const GridCell cell)
        {
            return static_cast<std::size_t>(cell.row) * map.columns()
                + cell.column;
        }

        [[nodiscard]] bool holdsTag(
            const std::vector<std::string> &held,
            const std::string_view tag)
        {
            return std::ranges::find(held, tag) != held.end();
        }

        [[nodiscard]] bool isWalkable(
            const Slab &slab, const std::vector<std::string> &held)
        {
            switch (slab.terrain)
            {
                case TerrainClass::Floor:
                case TerrainClass::Path:
                case TerrainClass::Stair:
                    return true;
                case TerrainClass::Water:
                    return slab.overlay == Overlay::Bridge
                        || (slab.water.swimmable
                            && holdsTag(held, kSwimTag));
                default:
                    return false;
            }
        }

        [[nodiscard]] FlowDirection opposite(const FlowDirection direction)
        {
            const auto index = static_cast<std::size_t>(direction);
            return static_cast<FlowDirection>(
                (index + kDirections.size() / 2) % kDirections.size());
        }

        [[nodiscard]] std::optional<GridCell> offsetCell(
            const TileMap &map,
            const GridCell from,
            const FlowDirection direction)
        {
            switch (direction)
            {
                case FlowDirection::North:
                    if (from.row == 0)
                    {
                        return std::nullopt;
                    }
                    return GridCell{
                        .column = from.column, .row = from.row - 1};
                case FlowDirection::East:
                    if (from.column + 1 >= map.columns())
                    {
                        return std::nullopt;
                    }
                    return GridCell{
                        .column = from.column + 1, .row = from.row};
                case FlowDirection::South:
                    if (from.row + 1 >= map.rows())
                    {
                        return std::nullopt;
                    }
                    return GridCell{
                        .column = from.column, .row = from.row + 1};
                default:
                    if (from.column == 0)
                    {
                        return std::nullopt;
                    }
                    return GridCell{
                        .column = from.column - 1, .row = from.row};
            }
        }

        [[nodiscard]] std::optional<FlowDirection> currentOf(
            const Slab &slab)
        {
            if (slab.terrain != TerrainClass::Water
                || slab.overlay == Overlay::Bridge)
            {
                return std::nullopt;
            }
            return slab.water.current;
        }

        [[nodiscard]] std::vector<std::size_t> surfaceOffsets(
            const TileMap &map)
        {
            const auto cells =
                static_cast<std::size_t>(map.columns()) * map.rows();
            std::vector<std::size_t> first(cells + 1, 0);
            for (std::uint32_t row = 0; row < map.rows(); ++row)
            {
                for (std::uint32_t column = 0;
                     column < map.columns();
                     ++column)
                {
                    const GridCell cell{.column = column, .row = row};
                    const auto index = cellIndexOf(map, cell);
                    first[index + 1] =
                        first[index] + map.at(cell).slabs().size();
                }
            }
            return first;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] std::size_t surfaceIdOf(
            const TileMap &map,
            const std::vector<std::size_t> &first,
            const Surface surface)
        {
            return first[cellIndexOf(map, surface.cell)] + surface.slab;
        }

        [[nodiscard]] std::optional<std::size_t> slabIndexAt(
            const Column &column, const std::int32_t level)
        {
            const auto &slabs = column.slabs();
            for (std::size_t index = 0; index < slabs.size(); ++index)
            {
                if (slabs[index].level == level)
                {
                    return index;
                }
            }
            return std::nullopt;
        }

        [[nodiscard]] std::optional<Surface> footingAt(
            const TileMap &map,
            const GridCell cell,
            const std::int32_t level,
            const std::vector<std::string> &held)
        {
            if (!inBounds(map, cell))
            {
                return std::nullopt;
            }
            const auto &column = map.at(cell);
            const auto slab = slabIndexAt(column, level);
            if (!slab.has_value()
                || !column.standable(level)
                || !isWalkable(column.slabs()[*slab], held))
            {
                return std::nullopt;
            }
            return Surface{.cell = cell, .slab = *slab};
        }

        [[nodiscard]] std::size_t indexWithin(
            const Column &column, const Slab &slab)
        {
            return static_cast<std::size_t>(
                &slab - column.slabs().data());
        }

        void appendTargets(
            const TileMap &map,
            const GridCell from,
            const Slab &source,
            const FlowDirection direction,
            const std::vector<std::string> &held,
            std::vector<Surface> &out)
        {
            const auto to = offsetCell(map, from, direction);
            if (!to.has_value() || !isWalkable(source, held))
            {
                return;
            }

            const auto current = currentOf(source);
            if (current == opposite(direction))
            {
                return;
            }

            const auto &column = map.at(*to);
            const auto *landing = column.topAtOrBelow(source.level);
            if (landing != nullptr
                && column.standable(landing->level)
                && isWalkable(*landing, held))
            {
                out.push_back(Surface{
                    .cell = *to,
                    .slab = indexWithin(column, *landing)});
            }

            if (source.level
                == std::numeric_limits<std::int32_t>::max())
            {
                return;
            }
            const auto *riser = column.slabAt(source.level + 1);
            if (riser == nullptr
                || !column.standable(riser->level)
                || !isWalkable(*riser, held))
            {
                return;
            }
            if (source.terrain != TerrainClass::Stair
                && riser->terrain != TerrainClass::Stair
                && current != direction)
            {
                return;
            }
            out.push_back(Surface{
                .cell = *to,
                .slab = indexWithin(column, *riser)});
        }

        [[nodiscard]] std::vector<Surface> targetsFrom(
            const TileMap &map,
            const Surface from,
            const std::vector<std::string> &held)
        {
            std::vector<Surface> targets{};
            const auto &source = map.at(from.cell).slabs()[from.slab];
            for (const auto direction : kDirections)
            {
                appendTargets(
                    map, from.cell, source, direction, held, targets);
            }
            return targets;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] bool linksTo(
            const TileMap &map,
            const GridCell from,
            const Slab &source,
            const FlowDirection direction,
            const std::vector<std::string> &held,
            const Surface target)
        {
            std::vector<Surface> targets{};
            appendTargets(map, from, source, direction, held, targets);
            return std::ranges::find(targets, target) != targets.end();
        }

        [[nodiscard]] std::vector<bool> reachableFrom(
            const TileMap &map,
            const std::vector<std::size_t> &first,
            const GridCell entry,
            const std::int32_t entryLevel,
            const std::vector<std::string> &held)
        {
            std::vector<bool> seen(first.back(), false);
            const auto start = footingAt(map, entry, entryLevel, held);
            if (!start.has_value())
            {
                return seen;
            }

            std::vector<Surface> frontier{*start};
            seen[surfaceIdOf(map, first, *start)] = true;
            while (!frontier.empty())
            {
                const auto from = frontier.back();
                frontier.pop_back();
                for (const auto &target : targetsFrom(map, from, held))
                {
                    const auto id = surfaceIdOf(map, first, target);
                    if (seen[id])
                    {
                        continue;
                    }
                    seen[id] = true;
                    frontier.push_back(target);
                }
            }
            return seen;
        }

        [[nodiscard]] std::vector<bool> returnersTo(
            const TileMap &map,
            const std::vector<std::size_t> &first,
            const GridCell entry,
            const std::int32_t entryLevel,
            const std::vector<std::string> &held)
        {
            std::vector<bool> seen(first.back(), false);
            const auto start = footingAt(map, entry, entryLevel, held);
            if (!start.has_value())
            {
                return seen;
            }

            std::vector<Surface> frontier{*start};
            seen[surfaceIdOf(map, first, *start)] = true;
            while (!frontier.empty())
            {
                const auto to = frontier.back();
                frontier.pop_back();
                for (const auto direction : kDirections)
                {
                    const auto from =
                        offsetCell(map, to.cell, direction);
                    if (!from.has_value())
                    {
                        continue;
                    }
                    const auto &column = map.at(*from);
                    const auto base = first[cellIndexOf(map, *from)];
                    for (std::size_t slab = 0;
                         slab < column.slabs().size();
                         ++slab)
                    {
                        if (seen[base + slab]
                            || !linksTo(
                                map,
                                *from,
                                column.slabs()[slab],
                                opposite(direction),
                                held,
                                to))
                        {
                            continue;
                        }
                        seen[base + slab] = true;
                        frontier.push_back(
                            Surface{.cell = *from, .slab = slab});
                    }
                }
            }
            return seen;
        }

        [[nodiscard]] GridCell cellOf(const Entity &entity)
        {
            return std::visit(
                [](const auto &one) { return one.at; }, entity);
        }

        [[nodiscard]] std::int32_t levelOf(const Entity &entity)
        {
            return std::visit(
                [](const auto &one) { return one.level; }, entity);
        }

        [[nodiscard]] std::string idOf(const Entity &entity)
        {
            return std::visit(
                [](const auto &one) { return one.id; }, entity);
        }

        [[nodiscard]] const std::vector<std::string> *grantsOf(
            const Entity &entity)
        {
            if (const auto *pickup = std::get_if<Pickup>(&entity))
            {
                return &pickup->grantedTags;
            }
            if (const auto *volume = std::get_if<TriggerVolume>(&entity))
            {
                return &volume->grantedTags;
            }
            return nullptr;
        }

        [[nodiscard]] bool covered(
            const TileMap &map,
            const std::vector<std::size_t> &first,
            const std::vector<bool> &reached,
            const TriggerVolume &volume)
        {
            for (std::uint32_t row = 0; row < volume.rows; ++row)
            {
                for (std::uint32_t column = 0;
                     column < volume.columns;
                     ++column)
                {
                    const GridCell cell{
                        .column = volume.at.column + column,
                        .row = volume.at.row + row};
                    if (!inBounds(map, cell))
                    {
                        continue;
                    }
                    const auto index = cellIndexOf(map, cell);
                    for (auto id = first[index];
                         id < first[index + 1];
                         ++id)
                    {
                        if (reached[id])
                        {
                            return true;
                        }
                    }
                }
            }
            return false;
        }

        [[nodiscard]] bool isReached(
            const TileMap &map,
            const std::vector<std::size_t> &first,
            const std::vector<bool> &reached,
            const std::vector<std::string> &held,
            const Entity &entity)
        {
            if (const auto *volume = std::get_if<TriggerVolume>(&entity))
            {
                return covered(map, first, reached, *volume);
            }
            const auto footing = footingAt(
                map, cellOf(entity), levelOf(entity), held);
            return footing.has_value()
                && reached[surfaceIdOf(map, first, *footing)];
        }

        [[nodiscard]] bool collectGrants(
            const TileMap &map,
            const std::vector<std::size_t> &first,
            const std::vector<bool> &reached,
            std::vector<std::string> &held)
        {
            bool grew = false;
            for (const auto &entity : map.entities())
            {
                const auto *granted = grantsOf(entity);
                if (granted == nullptr
                    || !isReached(map, first, reached, held, entity))
                {
                    continue;
                }
                for (const auto &tag : *granted)
                {
                    if (holdsTag(held, tag))
                    {
                        continue;
                    }
                    held.push_back(tag);
                    grew = true;
                }
            }
            return grew;
        }

        void reportUnreachable(
            const TileMap &map,
            const std::vector<std::size_t> &first,
            const std::vector<bool> &reached,
            const std::vector<std::string> &held,
            std::vector<Finding> &findings)
        {
            for (const auto &entity : map.entities())
            {
                const auto cell = cellOf(entity);
                const auto level = levelOf(entity);
                if (std::get_if<TriggerVolume>(&entity) == nullptr
                    && !footingAt(map, cell, level, held).has_value())
                {
                    findings.push_back(Finding{ // GCOVR_EXCL_LINE
                        .message = "entity " // GCOVR_EXCL_LINE
                            + idOf(entity) // GCOVR_EXCL_LINE
                            + " rests on no standable surface",
                        .at = cell,
                        .level = level});
                }
                if (isReached(map, first, reached, held, entity))
                {
                    continue;
                }
                findings.push_back(Finding{ // GCOVR_EXCL_LINE
                    .message = "entity " // GCOVR_EXCL_LINE
                        + idOf(entity) // GCOVR_EXCL_LINE
                        + " is unreachable", // GCOVR_EXCL_LINE
                    .at = cell,
                    .level = level});
            }
        }

        void reportGates(
            const TileMap &map,
            const std::vector<std::size_t> &first,
            const std::vector<bool> &reached,
            const std::vector<std::string> &held,
            std::vector<Finding> &findings)
        {
            for (const auto &entity : map.entities())
            {
                const auto *transition = std::get_if<Transition>(&entity);
                if (transition == nullptr
                    || !isReached(map, first, reached, held, entity))
                {
                    continue;
                }
                for (const auto &tag : transition->requiredTags)
                {
                    if (holdsTag(held, tag))
                    {
                        continue;
                    }
                    findings.push_back(Finding{ // GCOVR_EXCL_LINE
                        .message = "gate " // GCOVR_EXCL_LINE
                        + transition->id // GCOVR_EXCL_LINE
                        + " requires tag never granted: " // GCOVR_EXCL_LINE
                        + tag, // GCOVR_EXCL_LINE
                        .at = transition->at,
                        .level = transition->level});
                }
            }
        }

        [[nodiscard]] std::size_t fillRegion(
            const TileMap &map,
            const std::vector<std::size_t> &first,
            const std::vector<std::string> &held,
            const std::vector<bool> &dead,
            std::vector<bool> &seen,
            const Surface seed)
        {
            std::vector<Surface> frontier{seed};
            seen[surfaceIdOf(map, first, seed)] = true;
            std::size_t count = 0;
            while (!frontier.empty())
            {
                const auto surface = frontier.back();
                frontier.pop_back();
                ++count;
                for (const auto direction : kDirections)
                {
                    const auto from =
                        offsetCell(map, surface.cell, direction);
                    if (!from.has_value())
                    {
                        continue;
                    }
                    const auto &column = map.at(*from);
                    const auto base = first[cellIndexOf(map, *from)];
                    for (std::size_t slab = 0;
                         slab < column.slabs().size();
                         ++slab)
                    {
                        if (seen[base + slab]
                            || !dead[base + slab]
                            || !linksTo(
                                map,
                                *from,
                                column.slabs()[slab],
                                opposite(direction),
                                held,
                                surface))
                        {
                            continue;
                        }
                        seen[base + slab] = true;
                        frontier.push_back(
                            Surface{.cell = *from, .slab = slab});
                    }
                }
                for (const auto &target :
                     targetsFrom(map, surface, held))
                {
                    const auto id = surfaceIdOf(map, first, target);
                    if (seen[id])
                    {
                        continue;
                    }
                    seen[id] = true;
                    frontier.push_back(target);
                }
            }
            return count;
        }

        void reportDeadEnds(
            const TileMap &map,
            const std::vector<std::size_t> &first,
            const GridCell entry,
            const std::int32_t entryLevel,
            const std::vector<std::string> &held,
            const std::vector<bool> &reached,
            std::vector<Finding> &findings)
        {
            const auto returners =
                returnersTo(map, first, entry, entryLevel, held);
            std::vector<bool> dead(reached.size(), false);
            for (std::size_t index = 0; index < reached.size(); ++index)
            {
                dead[index] = reached[index] && !returners[index];
            }

            std::vector<bool> seen(reached.size(), false);
            for (std::uint32_t row = 0; row < map.rows(); ++row)
            {
                for (std::uint32_t column = 0;
                     column < map.columns();
                     ++column)
                {
                    const GridCell cell{.column = column, .row = row};
                    const auto base = first[cellIndexOf(map, cell)];
                    const auto &slabs = map.at(cell).slabs();
                    for (std::size_t slab = 0;
                         slab < slabs.size();
                         ++slab)
                    {
                        if (!dead[base + slab] || seen[base + slab])
                        {
                            continue;
                        }
                        const auto count = fillRegion(
                            map,
                            first,
                            held,
                            dead,
                            seen,
                            Surface{.cell = cell, .slab = slab});
                        findings.push_back(Finding{ // GCOVR_EXCL_LINE
                            .message = "dead end region of " // GCOVR_EXCL_LINE
                                + std::to_string(count) // GCOVR_EXCL_LINE
                                + (count == 1
                                    ? " surface"
                                    : " surfaces"),
                            .at = cell,
                            .level = slabs[slab].level});
                    }
                }
            }
        }

        [[nodiscard]] std::vector<CellReach> reachOf(
            const TileMap &map,
            const std::vector<std::size_t> &first,
            const std::vector<bool> &reached,
            const std::vector<std::string> &held)
        {
            std::vector<CellReach> reach(first.size() - 1);
            for (std::uint32_t row = 0; row < map.rows(); ++row)
            {
                for (std::uint32_t column = 0;
                     column < map.columns();
                     ++column)
                {
                    const GridCell cell{.column = column, .row = row};
                    const auto index = cellIndexOf(map, cell);
                    const auto &slabs = map.at(cell).slabs();
                    auto &one = reach[index];
                    for (std::size_t slab = 0;
                         slab < slabs.size();
                         ++slab)
                    {
                        if (map.at(cell).standable(slabs[slab].level)
                            && isWalkable(slabs[slab], held))
                        {
                            one.anyStandable = true;
                        }
                        if (reached[first[index] + slab])
                        {
                            one.anyReached = true;
                        }
                    }
                }
            }
            return reach;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] const TileMap *findMap(
            const std::vector<std::pair<std::string, TileMap>> &maps,
            const std::string &name)
        {
            for (const auto &[candidate, map] : maps)
            {
                if (candidate == name)
                {
                    return &map;
                }
            }
            return nullptr;
        }

        [[nodiscard]] const Transition *findTransition(
            const TileMap &map, const std::string &id)
        {
            for (const auto &entity : map.entities())
            {
                const auto *transition = std::get_if<Transition>(&entity);
                if (transition != nullptr && transition->id == id)
                {
                    return transition;
                }
            }
            return nullptr;
        }

        void checkTransition(
            const std::vector<std::pair<std::string, TileMap>> &maps,
            const std::string &name,
            const Transition &transition,
            std::vector<Finding> &findings)
        {
            const auto *target = findMap(maps, transition.targetMap);
            if (target == nullptr)
            {
                findings.push_back(Finding{ // GCOVR_EXCL_LINE
                    .map = name,
                    .message = "transition " // GCOVR_EXCL_LINE
                        + transition.id // GCOVR_EXCL_LINE
                        + " targets missing map " // GCOVR_EXCL_LINE
                        + transition.targetMap, // GCOVR_EXCL_LINE
                    .at = transition.at,
                    .level = transition.level});
                return;
            }

            const auto *counterpart =
                findTransition(*target, transition.targetEntry);
            if (counterpart == nullptr)
            {
                findings.push_back(Finding{ // GCOVR_EXCL_LINE
                    .map = name,
                    .message = "transition " // GCOVR_EXCL_LINE
                        + transition.id // GCOVR_EXCL_LINE
                        + " targets missing entry " // GCOVR_EXCL_LINE
                        + transition.targetEntry // GCOVR_EXCL_LINE
                        + " in " // GCOVR_EXCL_LINE
                        + transition.targetMap, // GCOVR_EXCL_LINE
                    .at = transition.at,
                    .level = transition.level});
                return;
            }

            if (counterpart->targetMap != name
                || counterpart->targetEntry != transition.id)
            {
                findings.push_back(Finding{ // GCOVR_EXCL_LINE
                    .map = name,
                    .message = "transition " // GCOVR_EXCL_LINE
                        + transition.id // GCOVR_EXCL_LINE
                        + " counterpart " // GCOVR_EXCL_LINE
                        + transition.targetEntry // GCOVR_EXCL_LINE
                        + " in " // GCOVR_EXCL_LINE
                        + transition.targetMap // GCOVR_EXCL_LINE
                        + " does not lead back", // GCOVR_EXCL_LINE
                    .at = transition.at,
                    .level = transition.level});
            }
        }
    }

    MapReport validateMap(
        const tilemap::TileMap &map,
        const geometry::GridCell entry,
        const std::int32_t entryLevel,
        const std::vector<std::string> &grantedTags)
    {
        const auto first = surfaceOffsets(map);
        auto held = grantedTags;
        auto reached =
            reachableFrom(map, first, entry, entryLevel, held);
        while (collectGrants(map, first, reached, held))
        {
            reached =
                reachableFrom(map, first, entry, entryLevel, held);
        }

        MapReport report{};

        if (!footingAt(map, entry, entryLevel, held).has_value())
        {
            report.findings.push_back(Finding{ // GCOVR_EXCL_LINE
                .message = "entry surface is not walkable", // GCOVR_EXCL_LINE
                .at = entry,
                .level = entryLevel});
        }

        reportUnreachable(map, first, reached, held, report.findings);
        reportGates(map, first, reached, held, report.findings);
        reportDeadEnds(
            map, first, entry, entryLevel, held, reached,
            report.findings);

        report.reachable = reachOf(map, first, reached, held);
        return report;
    }

    std::vector<Finding> validateWorld(
        const std::vector<std::pair<std::string, tilemap::TileMap>> &maps)
    {
        std::vector<Finding> findings{};
        for (const auto &[name, map] : maps)
        {
            for (const auto &entity : map.entities())
            {
                const auto *transition = std::get_if<Transition>(&entity);
                if (transition == nullptr)
                {
                    continue;
                }
                checkTransition(maps, name, *transition, findings);
            }
        }
        return findings;
    } // GCOVR_EXCL_LINE

}
