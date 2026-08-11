#include "antwika/mapcheck_cli/CheckMaps.hpp"

#include <cstdint>
#include <string>
#include <utility>
#include <variant>

#include <antwika/geometry/Grid.hpp>
#include <antwika/mapcheck/Finding.hpp>
#include <antwika/mapcheck/Validate.hpp>
#include <antwika/tilemap/Entities.hpp>
#include <antwika/tilemap/MapFile.hpp>
#include <antwika/tilemap/TileMap.hpp>
#include <antwika/tilemap/TileMapError.hpp>

namespace antwika::mapcheck_cli
{

    namespace
    {
        constexpr geometry::GridCell kFallbackEntry{.column = 1, .row = 1};

        struct Entry final
        {
            geometry::GridCell at{};
            std::int32_t level = 0;
        };

        [[nodiscard]] Entry entryOf(const tilemap::TileMap &map)
        {
            for (const auto &entity : map.entities())
            {
                const auto *transition =
                    std::get_if<tilemap::Transition>(&entity);
                if (transition != nullptr)
                {
                    return Entry{
                        .at = transition->at,
                        .level = transition->level};
                }
            }
            if (kFallbackEntry.column >= map.columns()
                || kFallbackEntry.row >= map.rows())
            {
                return Entry{.at = kFallbackEntry};
            }
            const auto *top = map.at(kFallbackEntry).top();
            return Entry{
                .at = kFallbackEntry,
                .level = top == nullptr ? 0 : top->level};
        }

        void printFinding(
            std::ostream &out, const mapcheck::Finding &finding)
        {
            out << finding.map << ": " << finding.message;
            if (finding.at.has_value()) // GCOVR_EXCL_LINE
            {
                out << " (" << finding.at->column << ','
                    << finding.at->row << ')';
            }
            if (finding.level.has_value()) // GCOVR_EXCL_LINE
            {
                out << " level " << *finding.level;
            }
            out << '\n';
        }
    }

    bool checkMaps(
        const std::vector<std::filesystem::path> &paths,
        std::ostream &out)
    {
        std::vector<std::pair<std::string, tilemap::TileMap>> loaded{};
        bool clean = true;

        for (const auto &path : paths)
        {
            try
            {
                loaded.emplace_back(
                    path.stem().string(), tilemap::loadMapFile(path));
            }
            catch (const tilemap::TileMapError &error) // GCOVR_EXCL_LINE
            {
                out << path.stem().string() << ": " << error.what()
                    << '\n';
                clean = false;
            }
        }

        for (const auto &[name, map] : loaded)
        {
            const auto entry = entryOf(map);
            auto report =
                mapcheck::validateMap(map, entry.at, entry.level, {});
            for (auto &finding : report.findings)
            {
                finding.map = name;
                printFinding(out, finding);
                clean = false;
            }
        }

        for (const auto &finding : mapcheck::validateWorld(loaded))
        {
            printFinding(out, finding);
            clean = false;
        }

        return clean;
    }

}
