#include <nlohmann/json.hpp>

#include <array>
#include <string>
#include <string_view>
#include <utility>

#include <antwika/schema/MigrationChain.hpp>
#include <antwika/schema/Step.hpp>

#include <antwika/map/MapFile.hpp>

#include "MapFileShared2.hpp"

namespace
{

    [[nodiscard]] std::int64_t getWayOfSheet(const std::int64_t way)
    {
        switch (way)
        {
        case 1:
            return 4;
        case 2:
            return 0;
        case 3:
            return 6;
        default:
            return 2;
        }
    }

}

namespace antwika::map
{
    using namespace mapfile;

    namespace
    {
        void mapV31ToV32(nlohmann::json &document)
        {
            for (auto &decor :
                 document[std::string(kDecorKey)])
            {
                decor[std::string(kSpanKey)] =
                    nlohmann::json::array({1, 1});
                decor[std::string(kMembersKey)] =
                    nlohmann::json::array(
                        {decor[std::string(kTileKey)]});
            }
        }

        void mapV34ToV35(nlohmann::json &document)
        {
            document[std::string(kKeysKey)] =
                nlohmann::json::array();
            document[std::string(kDoorsKey)] =
                nlohmann::json::array();
            document[std::string(kCheckpointsKey)] =
                nlohmann::json::array();
            document[std::string(kExitLockedKey)] = false;
        }

        void mapV35ToV36(nlohmann::json &document)
        {
            for (auto &figure :
                 document[std::string(kFiguresKey)])
            {
                figure[std::string(kFigureLampKey)] = false;
            }
        }

        void mapV36ToV37(nlohmann::json &document)
        {
            for (auto &decor :
                 document[std::string(kDecorKey)])
            {
                decor.erase(std::string(kNameKey));
            }
        }

        void mapV38ToV39(nlohmann::json &document)
        {
            constexpr int kWholeWeight = 100;

            for (auto &decor :
                 document[std::string(kDecorKey)])
            {
                decor[std::string(kWeightKey)] =
                    kWholeWeight;
            }
        }

        void mapV39ToV40(nlohmann::json &document)
        {
            for (auto &decor :
                 document[std::string(kDecorKey)])
            {
                decor[std::string(kDecorLayerKey)] = 1;
            }
        }

        void mapV40ToV41(nlohmann::json &document)
        {
            auto roster =
                document[std::string(kFiguresKey)];

            for (auto &figure : roster)
            {
                figure[std::string(kCharacterPlayerKey)] =
                    false;

                auto &way = figure[std::string(kHomeKey)]
                                  [std::string(kWayKey)];

                way = getWayOfSheet(way.get<std::int64_t>());
            }

            const auto &walker =
                document[std::string(kWalkerKey)];

            if (!walker.is_null())
            {
                nlohmann::json hero;

                hero[std::string(kNameKey)] = "Player";
                hero[std::string(kHomeKey)] = walker;
                hero[std::string(kStopsKey)] =
                    nlohmann::json::array();
                hero[std::string(kLinesKey)] =
                    nlohmann::json::array();
                hero[std::string(kFigureLampKey)] = true;
                hero[std::string(kCharacterPlayerKey)] = true;
                roster.push_back(hero);
            }

            document[std::string(kCharactersKey)] = roster;
            document.erase(std::string(kFiguresKey));
            document.erase(std::string(kWalkerKey));
        }

        void mapV42ToV43(nlohmann::json &document)
        {
            for (auto &figure :
                 document[std::string(kCharactersKey)])
            {
                auto components = nlohmann::json::array();

                if (figure.value(std::string(kFigureLampKey), false))
                {
                    components.push_back("light::CarriedLight");
                }

                figure.erase(std::string(kFigureLampKey));
                figure[std::string(kComponentsKey)] =
                    std::move(components);
            }
        }

        void mapV43ToV44(nlohmann::json &document)
        {
            constexpr std::array<std::string_view, 6> everyCharacter{
                "collision::Position",
                "collision::Velocity",
                "character::AnimationState",
                "character::CharacterIndex",
                "component::Health",
                "component::Inventory"};

            for (auto &figure :
                 document[std::string(kCharactersKey)])
            {
                auto components = nlohmann::json::array();

                for (const auto componentName : everyCharacter)
                {
                    components.push_back(std::string(componentName));
                }

                if (figure.value(
                        std::string(kCharacterPlayerKey), false))
                {
                    components.push_back("collision::Player");
                    components.push_back("light::FillLight");
                }

                for (const auto &componentName :
                     figure[std::string(kComponentsKey)])
                {
                    components.push_back(componentName);
                }

                figure[std::string(kComponentsKey)] =
                    std::move(components);
            }
        }

        void mapV44ToV45(nlohmann::json &document)
        {
            constexpr std::array<
                std::pair<std::string_view, std::string_view>, 8>
                renamedComponents{
                    std::pair{"collision::Position", "component::Position"},
                    std::pair{"collision::Velocity", "component::Velocity"},
                    std::pair{"collision::Player", "component::Player"},
                    std::pair{
                        "character::AnimationState",
                        "component::AnimationState"},
                    std::pair{
                        "character::CharacterIndex",
                        "component::CharacterIndex"},
                    std::pair{
                        "character::Speaker", "component::Speaker"},
                    std::pair{
                        "light::CarriedLight",
                        "component::CarriedLight"},
                    std::pair{
                        "light::FillLight", "component::FillLight"}};

            for (auto &figure :
                 document[std::string(kCharactersKey)])
            {
                for (auto &componentName :
                     figure[std::string(kComponentsKey)])
                {
                    const auto carriedName =
                        componentName.get<std::string>();

                    for (const auto &[oldName, newName] :
                         renamedComponents)
                    {
                        if (carriedName == oldName)
                        {
                            componentName = std::string(newName);

                            break;
                        }
                    }
                }
            }
        }

        void mapV45ToV46(nlohmann::json &document)
        {
            constexpr std::string_view kLadderName = "ladder";

            const auto isLadder = [kLadderName](const nlohmann::json &row)
            {
                const auto namedKind = row.find(std::string(kKindKey));

                return namedKind != row.end() && namedKind->is_string()
                       && namedKind->get<std::string>() == kLadderName;
            };

            const auto withoutLadders =
                [&isLadder](const nlohmann::json &rows)
            {
                auto keptRows = nlohmann::json::array();

                for (const auto &row : rows)
                {
                    if (!isLadder(row))
                    {
                        keptRows.push_back(row);
                    }
                }

                return keptRows;
            };

            for (const auto key : {kVoxelsKey, kTileKindsKey})
            {
                const auto foundRows = document.find(std::string(key));

                if (foundRows == document.end() || !foundRows->is_array())
                {
                    continue;
                }

                *foundRows = withoutLadders(*foundRows);
            }

            const auto foundSettings =
                document.find(std::string(kSettingsKey));

            if (foundSettings != document.end() && foundSettings->is_object()
                && isLadder(*foundSettings))
            {
                (*foundSettings)[std::string(kKindKey)] = "normal";
            }
        }

        void mapV46ToV47(nlohmann::json &document)
        {
            constexpr std::array<std::string_view, 10> kBenchKeys{
                kToolKey,
                kDrawingKey,
                kViewKey,
                kKindKey,
                kTiesKey,
                kGridKey,
                kMarkerKey,
                kSightKey,
                kFollowingKey,
                kAboveHiddenKey};

            const auto settings = document.find(std::string(kSettingsKey));

            if (settings == document.end() || !settings->is_object())
            {
                return;
            }

            for (const auto key : kBenchKeys)
            {
                settings->erase(std::string(key));
            }
        }
    }

    namespace mapfile
    {
        void mapMigrationsV30To47(
            schema::MigrationList &migrations)
        {
            const std::array rows{
                MigrationRow{
                    .fromVersion = 30,
                    .toVersion = 31,
                    .name = "antwika::map: a map may group tiles into "
                    "variant families now",
                    .apply = createEmptyArrays({kFamiliesKey})},
                MigrationRow{
                    .fromVersion = 31,
                    .toVersion = 32,
                    .name = "antwika::map: a decor may span several "
                    "tiles now",
                    .apply = mapV31ToV32},
                MigrationRow{
                    .fromVersion = 32,
                    .toVersion = 33,
                    .name = "antwika::map: a tile of the world may be "
                    "walked through frames now",
                    .apply = createEmptyArrays({kFlipsKey})},
                MigrationRow{
                    .fromVersion = 33,
                    .toVersion = 34,
                    .name = "antwika::map: a map may weave "
                    "transitions between materials now",
                    .apply = createEmptyArrays({kTransitionsKey})},
                MigrationRow{
                    .fromVersion = 34,
                    .toVersion = 35,
                    .name = "antwika::map: a map may hold keys, "
                    "doors and checkpoints now",
                    .apply = mapV34ToV35},
                MigrationRow{
                    .fromVersion = 35,
                    .toVersion = 36,
                    .name = "antwika::map: a figure may carry a lamp "
                    "now",
                    .apply = mapV35ToV36},
                MigrationRow{
                    .fromVersion = 36,
                    .toVersion = 37,
                    .name = "antwika::map: a decor goes unnamed now, "
                    "there being too many to call",
                    .apply = mapV36ToV37},
                MigrationRow{
                    .fromVersion = 37,
                    .toVersion = 38,
                    .name = "antwika::map: a tile now says which part "
                    "of a flight it was drawn for, its fronts "
                    "or its stepped side",
                    .apply = createEmptyArrays({kTilePartsKey})},
                MigrationRow{
                    .fromVersion = 38,
                    .toVersion = 39,
                    .name = "antwika::map: a decor now says how "
                    "strongly it is weighed against the others "
                    "its base offers",
                    .apply = mapV38ToV39},
                MigrationRow{
                    .fromVersion = 39,
                    .toVersion = 40,
                    .name = "antwika::map: a decor now says which "
                    "layer it dresses for, the layers laying "
                    "their decor over one another",
                    .apply = mapV39ToV40},
                MigrationRow{
                    .fromVersion = 40,
                    .toVersion = 41,
                    .name = "antwika::map: the figures and the walker "
                    "are one roster now, the player marked "
                    "among them",
                    .apply = mapV40ToV41},
                MigrationRow{
                    .fromVersion = 41,
                    .toVersion = 42,
                    .name = "antwika::map: a map may lay food and "
                    "water about for whoever walks it to pick "
                    "up",
                    .apply = createEmptyArrays({kFoodKey, kWaterKey})},
                MigrationRow{
                    .fromVersion = 42,
                    .toVersion = 43,
                    .name = "antwika::map: a figure names the "
                    "components it carries, the lamp flag "
                    "being the first of them",
                    .apply = mapV42ToV43},
                MigrationRow{
                    .fromVersion = 43,
                    .toVersion = 44,
                    .name = "antwika::map: a figure names every "
                    "component it spawns with, not only the "
                    "ones it carries",
                    .apply = mapV43ToV44},
                MigrationRow{
                    .fromVersion = 44,
                    .toVersion = 45,
                    .name = "antwika::map: every component a figure "
                    "names now lives in antwika::component",
                    .apply = mapV44ToV45},
                MigrationRow{
                    .fromVersion = 45,
                    .toVersion = 46,
                    .name = "antwika::map: a voxel is no longer climbed, "
                    "so the ladders a map stood and the rules "
                    "written for them are gone",
                    .apply = mapV45ToV46},
                MigrationRow{
                    .fromVersion = 46,
                    .toVersion = 47,
                    .name = "antwika::map: the tool, the view and the rest "
                    "of the workbench belong to whoever edits the "
                    "map, not to the map, and are kept beside it",
                    .apply = mapV46ToV47}};

            pushMigrations(migrations, rows);
        }
    }

}
