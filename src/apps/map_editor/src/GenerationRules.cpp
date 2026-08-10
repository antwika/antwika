#include "antwika/map_editor/GenerationRules.hpp"

#include <nlohmann/json.hpp>

#include <cstddef>
#include <fstream>
#include <utility>

#include <antwika/log/Level.hpp>

namespace antwika::map_editor
{

    namespace
    {
        using antwika::tilemap::TerrainClass;

        using Pair = std::pair<TerrainClass, TerrainClass>;

        constexpr std::array kForbidden{
            Pair{TerrainClass::Wall, TerrainClass::Water},
            Pair{TerrainClass::Wall, TerrainClass::Cliff},
            Pair{TerrainClass::Water, TerrainClass::Path},
            Pair{TerrainClass::Water, TerrainClass::Cliff},
            Pair{TerrainClass::Water, TerrainClass::Stair},
            Pair{TerrainClass::Path, TerrainClass::Cliff},
            Pair{TerrainClass::Cliff, TerrainClass::Stair},
        };

        [[nodiscard]] std::optional<TerrainClass> terrainNamed(
            const std::string &name)
        {
            for (const auto terrain :
                 enums::kAll<TerrainClass>)
            {
                if (tilemap::toString(terrain) == name)
                {
                    return terrain;
                }
            }

            return std::nullopt;
        }

        [[nodiscard]] bool generatable(const TerrainClass terrain)
        {
            return terrain != TerrainClass::Stair;
        }
    }

    GenerationRules defaultGenerationRules()
    {
        GenerationRules rules;

        for (auto &row : rules.allowed)
        {
            row.fill(true);
        }

        for (const auto &[left, right] : kForbidden)
        {
            const auto a = enums::index(left);
            const auto b = enums::index(right);

            rules.allowed[a][b] = false;
            rules.allowed[b][a] = false;
        }

        return rules;
    }

    std::optional<GenerationRules> rulesFromJson(
        const nlohmann::json &document)
    {
        if (!document.is_object()
            || !document.contains("weights")
            || !document.contains("adjacency")
            || !document.at("weights").is_object()
            || !document.at("adjacency").is_array())
        {
            return std::nullopt;
        }

        auto rules = defaultGenerationRules();

        for (auto &row : rules.allowed)
        {
            row.fill(false);
        }

        for (const auto &[name, value] :
             document.at("weights").items())
        {
            const auto terrain = terrainNamed(name);

            if (!terrain.has_value() || !generatable(*terrain)
                || !value.is_number()
                || value.get<double>() <= 0.0)
            {
                return std::nullopt;
            }

            rules.weights[enums::index(*terrain)] =
                value.get<double>();
        }

        for (const auto &entry : document.at("adjacency"))
        {
            if (!entry.is_array() || entry.size() != 2
                || !entry.at(0).is_string()
                || !entry.at(1).is_string())
            {
                return std::nullopt;
            }

            const auto left =
                terrainNamed(entry.at(0).get<std::string>());
            const auto right =
                terrainNamed(entry.at(1).get<std::string>());

            if (!left.has_value() || !right.has_value())
            {
                return std::nullopt;
            }

            const auto a = enums::index(*left);
            const auto b = enums::index(*right);

            rules.allowed[a][b] = true;
            rules.allowed[b][a] = true;
        }

        return rules;
    }

    nlohmann::json rulesToJson(const GenerationRules &rules)
    {
        nlohmann::json weights = nlohmann::json::object();

        for (const auto terrain : enums::kAll<TerrainClass>)
        {
            if (!generatable(terrain))
            {
                continue;
            }

            weights[std::string(tilemap::toString(terrain))] =
                rules.weights[enums::index(terrain)];
        }

        nlohmann::json adjacency = nlohmann::json::array();

        for (std::size_t a = 0; a < kTerrainCount; ++a)
        {
            for (std::size_t b = a; b < kTerrainCount; ++b)
            {
                if (!rules.allowed[a][b])
                {
                    continue;
                }

                adjacency.push_back(nlohmann::json::array(
                    {std::string(tilemap::toString(
                         static_cast<TerrainClass>(a))),
                     std::string(tilemap::toString(
                         static_cast<TerrainClass>(b)))}));
            }
        }

        nlohmann::json document;

        document["weights"] = std::move(weights);
        document["adjacency"] = std::move(adjacency);

        return document;
    }

    GenerationRules loadRulesFileOrDefaults(
        const std::filesystem::path &path, log::ILogger &logger)
    {
        std::ifstream in(path);

        if (!in)
        {
            logger.log(
                log::Level::Warning,
                "map_editor: no rules file at " + path.string()
                    + "; using the built-in rules");

            return defaultGenerationRules();
        }

        const auto document =
            nlohmann::json::parse(in, nullptr, false);
        const auto rules = document.is_discarded()
                               ? std::nullopt
                               : rulesFromJson(document);

        if (!rules.has_value())
        {
            logger.log(
                log::Level::Warning,
                "map_editor: corrupt rules file at "
                    + path.string()
                    + "; using the built-in rules");

            return defaultGenerationRules();
        }

        return *rules;
    }

    std::optional<std::string> saveRulesFile(
        const std::filesystem::path &path,
        const GenerationRules &rules)
    {
        std::ofstream out(path);

        if (!out)
        {
            return "cannot open " + path.string();
        }

        out << rulesToJson(rules).dump(2) << '\n';

        if (!out.good())
        {
            return "cannot write " + path.string();
        }

        return std::nullopt;
    }

}
