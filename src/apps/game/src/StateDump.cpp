#include "antwika/game/StateDump.hpp"

#include <cstddef>
#include <exception>
#include <memory>
#include <string>
#include <utility>

#include <antwika/replay/IMigration.hpp>
#include <antwika/replay/JsonShapes.hpp>
#include <antwika/enums/FromName.hpp>

#include "antwika/game/SaveFormatError.hpp"

namespace antwika::game
{

    namespace
    {
        using antwika::replay::wordShape;

        constexpr antwika::enums::NameTable<BuildTool>
            kTools{
                {"road",
                 "house",
                 "farm",
                 "clay_pit",
                 "workshop",
                 "storage",
                 "market",
                 "well",
                 "doctor",
                 "fire_station",
                 "engineer_post",
                 "raze"}};

        constexpr antwika::enums::NameTable<MapView>
            kViews{
                {"normal",
                 "desirability",
                 "food",
                 "water",
                 "medicine",
                 "fire",
                 "damage"}};

        nlohmann::json stateSchema()
        {
            nlohmann::json schema = antwika::replay::documentShape(
                "antwika game dump state",
                {"save", "paused", "mapView", "locale"});
            schema["properties"]["save"]["type"] = "object";
            schema["properties"]["paused"]["type"] = "boolean";
            schema["properties"]["tool"] = wordShape();
            schema["properties"]["mapView"] = wordShape();
            schema["properties"]["locale"] = wordShape();
            return schema;
        } // GCOVR_EXCL_LINE

        class DumpV1ToV2 final : public antwika::replay::IMigration
        {
        public:
            [[nodiscard]] std::uint32_t fromVersion() const noexcept
                override
            {
                return 1;
            }

            [[nodiscard]] std::uint32_t toVersion() const noexcept
                override
            {
                return 2;
            }

            // GCOVR_EXCL_START
            [[nodiscard]] std::string_view name() const noexcept override
            {
                return "game dump v1 -> v2: the shared envelope";
            }
            // GCOVR_EXCL_STOP

            void apply(nlohmann::json &document) const override
            {
                nlohmann::json state;

                for (const auto *member :
                     {"save", "paused", "tool", "mapView", "locale"})
                {
                    if (document.contains(member))
                    {
                        state[member] = std::move(document.at(member));
                        document.erase(member);
                    }
                }

                document["state"] = std::move(state);
            }
        };
    }

    antwika::replay::MigrationChain standardStateDumpMigrations()
    {
        antwika::replay::MigrationList migrations;
        migrations.push_back(
            std::make_shared<const DumpV1ToV2>()); // GCOVR_EXCL_LINE

        return antwika::replay::MigrationChain(
            std::move(migrations),
            kStateDumpVersion); // GCOVR_EXCL_LINE
    }

    nlohmann::json stateDumpToJson(const StateDump &dump)
    {
        nlohmann::json encoded;

        encoded["save"] = saveGameToJson(dump.save);
        encoded["paused"] = dump.paused;

        if (dump.tool.has_value())
        {
            encoded["tool"] = std::string(kTools.name(*dump.tool));
        }

        encoded["mapView"] = std::string(kViews.name(dump.view));
        encoded["locale"] =
            std::string(antwika::i18n::tagOf(dump.locale));

        return encoded;

    } // GCOVR_EXCL_LINE

    StateDump stateDumpFromJson(const nlohmann::json &state)
    {
        try
        {
            antwika::replay::validatorFor<stateSchema>().validate(
                state);
        }
        catch (const std::exception &failed) // GCOVR_EXCL_LINE
        {
            throw SaveFormatError(
                std::string(
                    "antwika::game: dump state failed schema "
                    "validation: ")
                + failed.what());
        }

        StateDump dump;

        dump.save = saveGameFromJson(state.at("save"));
        dump.paused = state.at("paused").get<bool>();

        if (state.contains("tool"))
        {
            dump.tool = antwika::enums::fromName<SaveFormatError>(
                kTools,
                state.at("tool").get<std::string>(),
                "antwika::game: dump names a tool this build does not "
                "know: ");
        }
        else
        {
            dump.tool = std::nullopt;
        }

        dump.view = antwika::enums::fromName<SaveFormatError>(
            kViews,
            state.at("mapView").get<std::string>(),
            "antwika::game: dump names a map view this build does not "
            "know: ");

        const auto tag = state.at("locale").get<std::string>();
        const auto locale = antwika::i18n::localeFromTag(tag);

        if (!locale.has_value())
        {
            throw SaveFormatError(
                "antwika::game: dump names a language this build "
                "has no catalogue for: "
                + tag);
        }

        dump.locale = *locale;

        return dump;
    }

}
