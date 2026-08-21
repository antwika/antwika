#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>

#include <antwika/schema/MigrationChain.hpp>
#include <antwika/schema/Step.hpp>

#include <antwika/map/MapFile.hpp>

#include "MapFileShared.hpp"

namespace antwika::map
{
    using namespace mapfile;

    namespace
    {
        void mapV22ToV23(nlohmann::json &document)
        {
            for (auto &decor :
                 document[std::string(kDecorKey)])
            {
                decor[std::string(kFrequencyKey)] =
                    decor::kFullFrequency;
            }
        }

        void mapV23ToV24(nlohmann::json &document)
        {
            for (auto &decor :
                 document[std::string(kDecorKey)])
            {
                decor[std::string(kNameKey)] = "";
            }
        }

        void mapV24ToV25(nlohmann::json &document)
        {
            auto &settings =
                document[std::string(kSettingsKey)];

            settings[std::string(kGridKey)] = true;
            settings[std::string(kMarkerKey)] = true;
            settings[std::string(kSightKey)] = true;
            settings[std::string(kFollowingKey)] = true;
            settings[std::string(kAboveHiddenKey)] = false;
            settings[std::string(kCornersJoinedKey)] = false;
        }

        void mapV25ToV26(nlohmann::json &document)
        {
            auto glows = nlohmann::json::array();

            for (std::size_t index = 0;
                 index < document[std::string(kPaletteKey)]
                          .size();
                 ++index)
            {
                glows.push_back(0);
            }

            document[std::string(kGlowsKey)] = glows;
            document[std::string(kAmbientKey)] = 0;
        }

        void mapV26ToV27(nlohmann::json &document)
        {
            (void)document;
        }

        void mapV27ToV28(nlohmann::json &document)
        {
            (void)document;
        }

        void mapV28ToV29(nlohmann::json &document)
        {
            document[std::string(kExitTargetKey)] = "";
        }

        void mapV29ToV30(nlohmann::json &document)
        {
            document[std::string(kFiguresKey)] =
                nlohmann::json::array();
            document[std::string(kPlatesKey)] =
                nlohmann::json::array();
        }
    }

    namespace mapfile
    {
        void newestMapMigrations(
            schema::MigrationList &migrations)
        {
            migrations.push_back(schema::step(
                22,
                23,
                "antwika::map: a decor now says how often "
                "it takes a base it may stand on",
                mapV22ToV23));
            migrations.push_back(schema::step(
                23,
                24,
                "antwika::map: a decor now carries a name "
                "the artist may give it",
                mapV23ToV24));
            migrations.push_back(schema::step(
                24,
                25,
                "antwika::map: a map now keeps the view "
                "toggles as they were left",
                mapV24ToV25));
            migrations.push_back(schema::step(
                25,
                26,
                "antwika::map: a map now says how its inks "
                "glow and how much light the world keeps",
                mapV25ToV26));
            migrations.push_back(schema::step(
                26,
                27,
                "antwika::map: a stamp tool joined the "
                "panel",
                mapV26ToV27));
            migrations.push_back(schema::step(
                27,
                28,
                "antwika::map: cubes may be ladders now",
                mapV27ToV28));
            migrations.push_back(schema::step(
                28,
                29,
                "antwika::map: an exit may lead to "
                "another map now",
                mapV28ToV29));
            migrations.push_back(schema::step(
                29,
                30,
                "antwika::map: a map may hold figures "
                "and plates now",
                mapV29ToV30));
        }
    }

}
