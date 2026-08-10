#include "antwika/mapcheck_cli/CheckMaps.hpp"

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

        [[nodiscard]] geometry::GridCell entryOf(
            const tilemap::TileMap &map)
        {
            for (const auto &entity : map.entities())
            {
                const auto *transition =
                    std::get_if<tilemap::Transition>(&entity);
                if (transition != nullptr)
                {
                    return transition->at;
                }
            }
            return kFallbackEntry;
        }

        void printFinding(
            std::ostream &out, const mapcheck::Finding &finding)
        {
            out << finding.map << ": " << finding.message;
            if (finding.at.has_value())
            {
                out << " (" << finding.at->column << ','
                    << finding.at->row << ')';
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
            catch (const tilemap::TileMapError &error)
            {
                out << path.stem().string() << ": " << error.what()
                    << '\n';
                clean = false;
            }
        }

        for (const auto &[name, map] : loaded)
        {
            auto report = mapcheck::validateMap(map, entryOf(map), {});
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
