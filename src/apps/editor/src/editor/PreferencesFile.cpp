#include "antwika/editor/PreferencesFile.hpp"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <ostream>
#include <string>
#include <string_view>

#include <antwika/enums/NameTable.hpp>
#include <antwika/io/File.hpp>
#include <antwika/map/MapFileError.hpp>
#include <antwika/map/MapFile.hpp>

namespace antwika::editor
{

    namespace
    {
        constexpr std::string_view kPreferencesName = "editor.json";

        constexpr enums::NameTable<map::Tool> kToolNames{
            {"brush",
             "picker",
             "lamp",
             "start",
             "exit",
             "stamp",
             "figure",
             "plate",
             "key",
             "door",
             "checkpoint",
             "food",
             "water",
             "eraser"}};

        static_assert(kToolNames.isComplete());

        constexpr enums::NameTable<map::Paint> kPaintNames{
            {"brush", "line", "fill", "select", "rect", "circle"}};

        static_assert(kPaintNames.isComplete());

        constexpr enums::NameTable<map::View> kViewNames{
            {"world", "atlases", "character", "icons", "plan"}};

        static_assert(kViewNames.isComplete());

        constexpr enums::NameTable<voxel::Kind> kKindNames{
            {"normal", "water", "ramp"}};

        static_assert(kKindNames.isComplete());

        template <typename Enum>
        [[nodiscard]] Enum enumOf(
            const nlohmann::json &document,
            const std::string_view key,
            const enums::NameTable<Enum> &names,
            const Enum whenMissing)
        {
            const auto foundName = document.find(std::string(key));

            if (foundName == document.end() || !foundName->is_string())
            {
                return whenMissing;
            }

            return names.getEnumFrom(foundName->get<std::string>())
                .value_or(whenMissing);
        }

        [[nodiscard]] bool isFlagOn(
            const nlohmann::json &document,
            const std::string_view key,
            const bool whenMissing)
        {
            const auto foundFlag = document.find(std::string(key));

            return foundFlag != document.end() && foundFlag->is_boolean()
                       ? foundFlag->get<bool>()
                       : whenMissing;
        }
    }

    std::string getPreferencesPath(const std::string &mapPath)
    {
        return map::getSidecarPath(mapPath, kPreferencesName);
    }

    Preferences getLoadPreferences(const std::string &mapPath)
    {
        const Preferences restingPreferences;
        const auto path = getPreferencesPath(mapPath);

        if (!std::filesystem::exists(path))
        {
            return restingPreferences;
        }

        std::ifstream reading(path);
        nlohmann::json document;

        try
        {
            reading >> document;
        }
        catch (const nlohmann::json::exception &)
        {
            return restingPreferences;
        }

        if (!document.is_object())
        {
            return restingPreferences;
        }

        return Preferences{
            .tool = enumOf(document, "tool", kToolNames, restingPreferences.tool),
            .paint = enumOf(document, "paint", kPaintNames, restingPreferences.paint),
            .view = enumOf(document, "view", kViewNames, restingPreferences.view),
            .kind = enumOf(document, "kind", kKindNames, restingPreferences.kind),
            .showRuleLines =
                isFlagOn(document, "ties", restingPreferences.showRuleLines),
            .grid = isFlagOn(document, "grid", restingPreferences.grid),
            .showPlacementGhost =
                isFlagOn(document, "marker", restingPreferences.showPlacementGhost),
            .lampSight = isFlagOn(document, "sight", restingPreferences.lampSight),
            .cameraFollows =
                isFlagOn(document, "following", restingPreferences.cameraFollows),
            .hideAboveLevel =
                isFlagOn(document, "aboveHidden", restingPreferences.hideAboveLevel)};
    }

    void savePreferences(
        const std::string &mapPath, const Preferences &preferences)
    {
        nlohmann::json document;

        document["tool"] = std::string(kToolNames.getName(preferences.tool));
        document["paint"] =
            std::string(kPaintNames.getName(preferences.paint));
        document["view"] = std::string(kViewNames.getName(preferences.view));
        document["kind"] = std::string(kKindNames.getName(preferences.kind));
        document["ties"] = preferences.showRuleLines;
        document["grid"] = preferences.grid;
        document["marker"] = preferences.showPlacementGhost;
        document["sight"] = preferences.lampSight;
        document["following"] = preferences.cameraFollows;
        document["aboveHidden"] = preferences.hideAboveLevel;

        const auto path = getPreferencesPath(mapPath);
        auto writing = io::openToWriteAs<map::MapFileError>(
            path, "the editor settings");

        writing << document.dump(2) << "\n";

        io::requireStreamOkAs<map::MapFileError>(
            writing, "the editor settings", path);
    }

}
