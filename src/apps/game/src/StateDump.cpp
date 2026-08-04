#include "antwika/game/StateDump.hpp"

#include <array>
#include <cstddef>
#include <exception>
#include <memory>
#include <string>
#include <utility>

#include <nlohmann/json-schema.hpp>

#include <antwika/replay/IMigration.hpp>

#include "antwika/game/SaveFormatError.hpp"

namespace antwika::game
{

    namespace
    {
        // The names a dump document holds, one per tool.
        // Persisted, so they may not change once written.
        // The same rule buildingKindName() is held to.
        constexpr std::array<std::string_view, kBuildToolCount>
            kToolNames{
                "road",
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
                "raze"};

        // The names a dump document holds, one per view.
        constexpr std::array<std::string_view, kMapViewCount> kViewNames{
            "normal",
            "desirability",
            "food",
            "water",
            "medicine",
            "fire",
            "damage"};

        [[nodiscard]] std::string_view toolName(BuildTool tool) noexcept
        {
            return kToolNames[buildToolIndex(tool) % kBuildToolCount];
        }

        [[nodiscard]] std::optional<BuildTool> toolFromName(
            std::string_view name) noexcept
        {
            for (std::size_t index = 0; index < kBuildToolCount; ++index)
            {
                if (kToolNames[index] == name)
                {
                    return static_cast<BuildTool>(index);
                }
            }

            return std::nullopt;
        }

        [[nodiscard]] std::string_view viewName(MapView view) noexcept
        {
            return kViewNames[mapViewIndex(view) % kMapViewCount];
        }

        [[nodiscard]] std::optional<MapView> viewFromName(
            std::string_view name) noexcept
        {
            for (std::size_t index = 0; index < kMapViewCount; ++index)
            {
                if (kViewNames[index] == name)
                {
                    return static_cast<MapView>(index);
                }
            }

            return std::nullopt;
        }

        nlohmann::json stateSchema()
        {
            // The save is a whole versioned document of its own.
            // saveGameFromJson() migrates and validates it itself.
            // This schema only says that one is there.
            nlohmann::json schema;
            schema["$schema"] = "http://json-schema.org/draft-07/schema#";
            schema["title"] = "antwika game dump state";
            schema["type"] = "object";
            schema["additionalProperties"] = false;
            schema["required"] = {
                "save", "paused", "mapView", "locale"}; // GCOVR_EXCL_LINE
            schema["properties"]["save"]["type"] = "object";
            schema["properties"]["paused"]["type"] = "boolean";
            schema["properties"]["tool"]["type"] = "string";
            schema["properties"]["mapView"]["type"] = "string";
            schema["properties"]["locale"]["type"] = "string";
            return schema;
        }

        const nlohmann::json_schema::json_validator &stateValidator()
        {
            // The excluded closing line carries the static guard.
            // Its concurrency arms are unreachable one-threaded.
            // See docs/confirming-unreachable-branches.md.
            static const nlohmann::json_schema::json_validator validator(
                stateSchema()); // GCOVR_EXCL_LINE
            return validator;
        }

        // Version 2 moved the state under the shared envelope.
        // A version 1 document was this application's own shape.
        // Its members move under "state" and mean what they meant.
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

            // Only ever read to word a MigrationChain's refusal.
            // OptionsV1ToV2 says why no input can reach it.
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
    } // namespace

    antwika::replay::MigrationChain standardStateDumpMigrations()
    {
        // Every branch left on the excluded lines is the allocator's.
        // The unwind edges of building a list of one shared_ptr.
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

        // Absent means the palette was down.
        // A member for it would be a name for no tool.
        if (dump.tool.has_value())
        {
            encoded["tool"] = std::string(toolName(*dump.tool));
        }

        encoded["mapView"] = std::string(viewName(dump.view));
        encoded["locale"] =
            std::string(antwika::i18n::tagOf(dump.locale));

        return encoded;

        // gcov puts the returned value's unwind block here.
        // SaveGame.cpp's own encoder explains it at length.
        // No input reaches it.
    } // GCOVR_EXCL_LINE

    StateDump stateDumpFromJson(const nlohmann::json &state)
    {
        try
        {
            stateValidator().validate(state);
        }
        // The validator's failure type is the library's business.
        // What this format promises is SaveFormatError.
        // So it is rewrapped here, as keyNamed() rewraps a key's.
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
            const auto named = state.at("tool").get<std::string>();
            const auto tool = toolFromName(named);

            if (!tool.has_value())
            {
                throw SaveFormatError(
                    "antwika::game: dump names a tool this build "
                    "does not know: "
                    + named);
            }

            dump.tool = tool;
        }
        else
        {
            dump.tool = std::nullopt;
        }

        const auto viewNamed = state.at("mapView").get<std::string>();
        const auto view = viewFromName(viewNamed);

        if (!view.has_value())
        {
            throw SaveFormatError(
                "antwika::game: dump names a map view this build "
                "does not know: "
                + viewNamed);
        }

        dump.view = *view;

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

} // namespace antwika::game
