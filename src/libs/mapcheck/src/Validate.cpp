#include "antwika/mapcheck/Validate.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

#include <antwika/tilemap/Cell.hpp>
#include <antwika/tilemap/Entities.hpp>
#include <antwika/tilemap/FlowDirection.hpp>
#include <antwika/tilemap/Overlay.hpp>
#include <antwika/tilemap/TerrainClass.hpp>

namespace antwika::mapcheck
{

    namespace
    {
        using geometry::GridCell;
        using tilemap::Cell;
        using tilemap::Entity;
        using tilemap::FlowDirection;
        using tilemap::Overlay;
        using tilemap::Pickup;
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

        [[nodiscard]] bool inBounds(const TileMap &map, const GridCell cell)
        {
            return cell.column < map.columns() && cell.row < map.rows();
        }

        [[nodiscard]] std::size_t indexOf(
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
            const Cell &cell, const std::vector<std::string> &held)
        {
            switch (cell.terrain)
            {
                case TerrainClass::Floor:
                case TerrainClass::Path:
                case TerrainClass::Stair:
                    return true;
                case TerrainClass::Water:
                    return cell.overlay == Overlay::Bridge
                        || (cell.water.swimmable
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
            const Cell &cell)
        {
            if (cell.terrain != TerrainClass::Water
                || cell.overlay == Overlay::Bridge)
            {
                return std::nullopt;
            }
            return cell.water.current;
        }

        [[nodiscard]] bool stepAllowed(const Cell &source, const Cell &target)
        {
            const auto rise = static_cast<std::int64_t>(target.height)
                - source.height;
            if (rise <= 0)
            {
                return true;
            }
            return rise == 1
                && (source.terrain == TerrainClass::Stair
                    || target.terrain == TerrainClass::Stair);
        }

        [[nodiscard]] bool hasEdge(
            const TileMap &map,
            const GridCell from,
            const FlowDirection direction,
            const std::vector<std::string> &held)
        {
            const auto to = offsetCell(map, from, direction);
            if (!to.has_value())
            {
                return false;
            }

            const auto &source = map.at(from);
            const auto &target = map.at(*to);
            if (!isWalkable(source, held) || !isWalkable(target, held))
            {
                return false;
            }

            const auto current = currentOf(source);
            if (current == direction)
            {
                return true;
            }
            if (current == opposite(direction))
            {
                return false;
            }
            return stepAllowed(source, target);
        }

        [[nodiscard]] std::vector<bool> reachableFrom(
            const TileMap &map,
            const GridCell entry,
            const std::vector<std::string> &held)
        {
            std::vector<bool> seen(
                static_cast<std::size_t>(map.columns()) * map.rows(),
                false);
            if (!inBounds(map, entry) || !isWalkable(map.at(entry), held))
            {
                return seen;
            }

            std::vector<GridCell> frontier{entry};
            seen[indexOf(map, entry)] = true;
            while (!frontier.empty())
            {
                const auto from = frontier.back();
                frontier.pop_back();
                for (const auto direction : kDirections)
                {
                    if (!hasEdge(map, from, direction, held))
                    {
                        continue;
                    }
                    const auto to = *offsetCell(map, from, direction);
                    const auto index = indexOf(map, to);
                    if (seen[index])
                    {
                        continue;
                    }
                    seen[index] = true;
                    frontier.push_back(to);
                }
            }
            return seen;
        }

        [[nodiscard]] std::vector<bool> returnersTo(
            const TileMap &map,
            const GridCell entry,
            const std::vector<std::string> &held)
        {
            std::vector<bool> seen(
                static_cast<std::size_t>(map.columns()) * map.rows(),
                false);
            if (!inBounds(map, entry) || !isWalkable(map.at(entry), held))
            {
                return seen;
            }

            std::vector<GridCell> frontier{entry};
            seen[indexOf(map, entry)] = true;
            while (!frontier.empty())
            {
                const auto to = frontier.back();
                frontier.pop_back();
                for (const auto direction : kDirections)
                {
                    const auto from = offsetCell(map, to, direction);
                    if (!from.has_value())
                    {
                        continue;
                    }
                    const auto index = indexOf(map, *from);
                    if (seen[index]
                        || !hasEdge(
                            map, *from, opposite(direction), held))
                    {
                        continue;
                    }
                    seen[index] = true;
                    frontier.push_back(*from);
                }
            }
            return seen;
        }

        [[nodiscard]] GridCell cellOf(const Entity &entity)
        {
            return std::visit(
                [](const auto &one) { return one.at; }, entity);
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
                    if (inBounds(map, cell)
                        && reached[indexOf(map, cell)])
                    {
                        return true;
                    }
                }
            }
            return false;
        }

        [[nodiscard]] bool isReached(
            const TileMap &map,
            const std::vector<bool> &reached,
            const Entity &entity)
        {
            if (const auto *volume = std::get_if<TriggerVolume>(&entity))
            {
                return covered(map, reached, *volume);
            }
            const auto cell = cellOf(entity);
            return inBounds(map, cell) && reached[indexOf(map, cell)];
        }

        [[nodiscard]] bool collectGrants(
            const TileMap &map,
            const std::vector<bool> &reached,
            std::vector<std::string> &held)
        {
            bool grew = false;
            for (const auto &entity : map.entities())
            {
                const auto *granted = grantsOf(entity);
                if (granted == nullptr
                    || !isReached(map, reached, entity))
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
            const std::vector<bool> &reached,
            std::vector<Finding> &findings)
        {
            for (const auto &entity : map.entities())
            {
                if (isReached(map, reached, entity))
                {
                    continue;
                }
                findings.push_back(Finding{
                    .message =
                        "entity " + idOf(entity) + " is unreachable",
                    .at = cellOf(entity)});
            }
        }

        void reportGates(
            const TileMap &map,
            const std::vector<bool> &reached,
            const std::vector<std::string> &held,
            std::vector<Finding> &findings)
        {
            for (const auto &entity : map.entities())
            {
                const auto *transition = std::get_if<Transition>(&entity);
                if (transition == nullptr
                    || !isReached(map, reached, entity))
                {
                    continue;
                }
                for (const auto &tag : transition->requiredTags)
                {
                    if (holdsTag(held, tag))
                    {
                        continue;
                    }
                    findings.push_back(Finding{
                        .message = "gate " + transition->id
                            + " requires tag never granted: " + tag,
                        .at = transition->at});
                }
            }
        }

        [[nodiscard]] std::size_t fillRegion(
            const TileMap &map,
            const std::vector<bool> &dead,
            std::vector<bool> &seen,
            const GridCell seed)
        {
            std::vector<GridCell> frontier{seed};
            seen[indexOf(map, seed)] = true;
            std::size_t count = 0;
            while (!frontier.empty())
            {
                const auto cell = frontier.back();
                frontier.pop_back();
                ++count;
                for (const auto direction : kDirections)
                {
                    const auto next = offsetCell(map, cell, direction);
                    if (!next.has_value())
                    {
                        continue;
                    }
                    const auto index = indexOf(map, *next);
                    if (seen[index] || !dead[index])
                    {
                        continue;
                    }
                    seen[index] = true;
                    frontier.push_back(*next);
                }
            }
            return count;
        }

        void reportDeadEnds(
            const TileMap &map,
            const GridCell entry,
            const std::vector<std::string> &held,
            const std::vector<bool> &reached,
            std::vector<Finding> &findings)
        {
            const auto returners = returnersTo(map, entry, held);
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
                    const auto index = indexOf(map, cell);
                    if (!dead[index] || seen[index])
                    {
                        continue;
                    }
                    const auto count = fillRegion(map, dead, seen, cell);
                    findings.push_back(Finding{
                        .message = "dead end region of "
                            + std::to_string(count)
                            + (count == 1 ? " cell" : " cells"),
                        .at = cell});
                }
            }
        }

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
                findings.push_back(Finding{
                    .map = name,
                    .message = "transition " + transition.id
                        + " targets missing map " + transition.targetMap,
                    .at = transition.at});
                return;
            }

            const auto *counterpart =
                findTransition(*target, transition.targetEntry);
            if (counterpart == nullptr)
            {
                findings.push_back(Finding{
                    .map = name,
                    .message = "transition " + transition.id
                        + " targets missing entry "
                        + transition.targetEntry
                        + " in " + transition.targetMap,
                    .at = transition.at});
                return;
            }

            if (counterpart->targetMap != name
                || counterpart->targetEntry != transition.id)
            {
                findings.push_back(Finding{
                    .map = name,
                    .message = "transition " + transition.id
                        + " counterpart " + transition.targetEntry
                        + " in " + transition.targetMap
                        + " does not lead back",
                    .at = transition.at});
            }
        }
    }

    MapReport validateMap(
        const tilemap::TileMap &map,
        const geometry::GridCell entry,
        const std::vector<std::string> &grantedTags)
    {
        auto held = grantedTags;
        auto reached = reachableFrom(map, entry, held);
        while (collectGrants(map, reached, held))
        {
            reached = reachableFrom(map, entry, held);
        }

        MapReport report{};

        if (!inBounds(map, entry) || !isWalkable(map.at(entry), held))
        {
            report.findings.push_back(Finding{
                .message = "entry cell is not walkable",
                .at = entry});
        }

        reportUnreachable(map, reached, report.findings);
        reportGates(map, reached, held, report.findings);
        reportDeadEnds(map, entry, held, reached, report.findings);

        report.reachable = std::move(reached);
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
    }

}
