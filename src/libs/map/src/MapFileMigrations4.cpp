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
        void mapV30ToV31(nlohmann::json &document)
        {
            document[std::string(kFamiliesKey)] =
                nlohmann::json::array();
        }
    }

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
    }

    namespace
    {
        void mapV32ToV33(nlohmann::json &document)
        {
            document[std::string(kFlipsKey)] =
                nlohmann::json::array();
        }
    }

    namespace
    {
        void mapV33ToV34(nlohmann::json &document)
        {
            document[std::string(kTransitionsKey)] =
                nlohmann::json::array();
        }
    }

    namespace
    {
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
    }

    namespace
    {
        void mapV35ToV36(nlohmann::json &document)
        {
            for (auto &figure :
                 document[std::string(kFiguresKey)])
            {
                figure[std::string(kFigureLampKey)] = false;
            }
        }
    }

    namespace
    {
        void mapV36ToV37(nlohmann::json &document)
        {
            for (auto &decor :
                 document[std::string(kDecorKey)])
            {
                decor.erase(std::string(kNameKey));
            }
        }

        void mapV37ToV38(nlohmann::json &document)
        {
            document[std::string(kTilePartsKey)] =
                nlohmann::json::array();
        }

        void mapV38ToV39(nlohmann::json &document)
        {
            for (auto &decor :
                 document[std::string(kDecorKey)])
            {
                decor[std::string(kWeightKey)] =
                    decor::kFullFrequency;
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
                        "character::RosterIndex",
                        "component::RosterIndex"},
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

        void mapV43ToV44(nlohmann::json &document)
        {
            constexpr std::array<std::string_view, 6> everyCharacter{
                "collision::Position",
                "collision::Velocity",
                "character::AnimationState",
                "character::RosterIndex",
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

        void mapV41ToV42(nlohmann::json &document)
        {
            document[std::string(kFoodKey)] =
                nlohmann::json::array();
            document[std::string(kWaterKey)] =
                nlohmann::json::array();
        }
    }

    namespace mapfile
    {
        schema::MigrationChain getMapMigrations()
        {
            schema::MigrationList migrations;

            earlyMapMigrations(migrations);
            lateMapMigrations(migrations);
            newestMapMigrations(migrations);
            latestMapMigrations(migrations);

            return schema::MigrationChain(
                std::move(migrations), kMapVersion);
        } // GCOVR_EXCL_LINE

        void latestMapMigrations(
            schema::MigrationList &migrations)
        {
            migrations.push_back(schema::getMigration(
                30,
                31,
                "antwika::map: a map may group tiles into "
                "variant families now",
                mapV30ToV31));
            migrations.push_back(schema::getMigration(
                31,
                32,
                "antwika::map: a decor may span several "
                "tiles now",
                mapV31ToV32));
            migrations.push_back(schema::getMigration(
                32,
                33,
                "antwika::map: a tile of the world may be "
                "walked through frames now",
                mapV32ToV33));
            migrations.push_back(schema::getMigration(
                33,
                34,
                "antwika::map: a map may weave "
                "transitions between materials now",
                mapV33ToV34));
            migrations.push_back(schema::getMigration(
                34,
                35,
                "antwika::map: a map may hold keys, "
                "doors and checkpoints now",
                mapV34ToV35));
            migrations.push_back(schema::getMigration(
                35,
                36,
                "antwika::map: a figure may carry a lamp "
                "now",
                mapV35ToV36));
            migrations.push_back(schema::getMigration(
                36,
                37,
                "antwika::map: a decor goes unnamed now, "
                "there being too many to call",
                mapV36ToV37));
            migrations.push_back(schema::getMigration(
                37,
                38,
                "antwika::map: a tile now says which part "
                "of a flight it was drawn for, its fronts "
                "or its stepped side",
                mapV37ToV38));
            migrations.push_back(schema::getMigration(
                38,
                39,
                "antwika::map: a decor now says how "
                "strongly it is weighed against the others "
                "its base offers",
                mapV38ToV39));
            migrations.push_back(schema::getMigration(
                39,
                40,
                "antwika::map: a decor now says which "
                "layer it dresses for, the layers laying "
                "their decor over one another",
                mapV39ToV40));
            migrations.push_back(schema::getMigration(
                40,
                41,
                "antwika::map: the figures and the walker "
                "are one roster now, the player marked "
                "among them",
                mapV40ToV41));
            migrations.push_back(schema::getMigration(
                41,
                42,
                "antwika::map: a map may lay food and "
                "water about for whoever walks it to pick "
                "up",
                mapV41ToV42));
            migrations.push_back(schema::getMigration(
                42,
                43,
                "antwika::map: a figure names the "
                "components it carries, the lamp flag "
                "being the first of them",
                mapV42ToV43));
            migrations.push_back(schema::getMigration(
                43,
                44,
                "antwika::map: a figure names every "
                "component it spawns with, not only the "
                "ones it carries",
                mapV43ToV44));
            migrations.push_back(schema::getMigration(
                44,
                45,
                "antwika::map: every component a figure "
                "names now lives in antwika::component",
                mapV44ToV45));
        }
    }

}
