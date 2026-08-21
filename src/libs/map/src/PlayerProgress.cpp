#include "antwika/map/PlayerProgress.hpp"

#include <nlohmann/json.hpp>

#include <fstream>

#include "MapFileShared.hpp"

namespace antwika::map
{
    using namespace mapfile;

    void saveProgress(
        const Progress &progress, const std::string &path)
    {
        nlohmann::json document;

        document[std::string(kMagicKey)] = "antwika.progress";
        document["map"] = progress.map;
        document[std::string(kAtKey)] = nlohmann::json::array(
            {toFixed(progress.stancePlacement.position.x),
             toFixed(progress.stancePlacement.position.y),
             toFixed(progress.stancePlacement.position.z)});
        document[std::string(kWayKey)] = progress.stancePlacement.way;

        std::ofstream outputStream(path);

        outputStream << document.dump(kIndent) << '\n';
    }

    std::optional<Progress> loadProgress(const std::string &path)
    {
        std::ifstream inputStream(path);

        if (!inputStream)
        {
            return std::nullopt;
        }

        nlohmann::json document;

        try
        {
            inputStream >> document;
        }
        catch (const nlohmann::json::exception &)
        {
            return std::nullopt;
        }

        if (!document.is_object() || !document.contains("map")
            || !document["map"].is_string()
            || !document.contains(std::string(kAtKey))
            || !document[std::string(kAtKey)].is_array()
            || document[std::string(kAtKey)].size() != 3
            || !document.contains(std::string(kWayKey))
            || !document[std::string(kWayKey)]
                    .is_number_unsigned())
        {
            return std::nullopt;
        }

        const auto &atJson = document[std::string(kAtKey)];

        return Progress{
            .map = document["map"].get<std::string>(),
            .stancePlacement = Placement{
                .position =
                    gfx::Vec3{
                        fromFixed(atJson[0].get<std::int64_t>()),
                        fromFixed(atJson[1].get<std::int64_t>()),
                        fromFixed(atJson[2].get<std::int64_t>())},
                .way = document[std::string(kWayKey)]
                           .get<std::uint8_t>()}};
    } // GCOVR_EXCL_LINE

}
