#include "antwika/companion/PetSave.hpp"

#include <array>
#include <cstddef>
#include <string>

#include <nlohmann/json-schema.hpp>

#include <antwika/replay/JsonShapes.hpp>
#include <antwika/replay/SchemaVersion.hpp>
#include <antwika/replay/VersionedDocument.hpp>

#include "antwika/companion/SaveFormatError.hpp"

namespace antwika::companion
{

    namespace
    {
        // Two spaces, one member a line.
        // Enough to diff a companion against the next version of itself.
        // That is the whole reason this format is not compact.
        constexpr int kIndent = 2;

        // Symbolic rather than the enumerator's number.
        // A name survives the enumeration being reordered.
        // Being hand-editable is most of why this format is JSON.
        // Indexed by the enumerator, so the order is the enum's.
        constexpr std::array<std::string_view, 3> kStateNames{
            "awake", "asleep", "perished"};

        static_assert(
            kStateNames.size()
            == static_cast<std::size_t>(PetState::Perished) + 1);

        constexpr std::array<std::string_view, 10> kSayingNames{
            "none",
            "hello",
            "bored",
            "niceDay",
            "silly",
            "feedMe",
            "yum",
            "notHungry",
            "letMeSleep",
            "zzz"};

        static_assert(
            kSayingNames.size()
            == static_cast<std::size_t>(Saying::Zzz) + 1);

        [[nodiscard]] std::string_view stateName(const PetState state)
        {
            return kStateNames[static_cast<std::size_t>(state)];
        }

        [[nodiscard]] PetState stateFromName(const std::string &name)
        {
            for (std::size_t index = 0; index < kStateNames.size();
                 ++index)
            {
                if (kStateNames[index] == name)
                {
                    return static_cast<PetState>(index);
                }
            }

            throw SaveFormatError(
                "antwika::companion: a saved companion names a state "
                "that is not one of the three: " + name);
        }

        [[nodiscard]] std::string_view sayingName(const Saying saying)
        {
            return kSayingNames[static_cast<std::size_t>(saying)];
        }

        [[nodiscard]] Saying sayingFromName(const std::string &name)
        {
            for (std::size_t index = 0; index < kSayingNames.size();
                 ++index)
            {
                if (kSayingNames[index] == name)
                {
                    return static_cast<Saying>(index);
                }
            }

            throw SaveFormatError(
                "antwika::companion: a saved companion says a line this "
                "build does not have: " + name);
        }

        // Both shapes are antwika::replay's.
        // All three formats stating a version share them.
        using replay::countShape;
        using replay::wordShape;

        // "state" and "saying" are strings rather than schema enums.
        // An unknown name is refused by the two functions above instead.
        // That way the message holds the name it did not know.
        nlohmann::json petSchema()
        {
            nlohmann::json schema;
            schema["$schema"] = "http://json-schema.org/draft-07/schema#";
            schema["title"] = "antwika companion document";
            schema["type"] = "object";
            schema["additionalProperties"] = false;

            // The version member is described but not required.
            // A document without one is read as version 1 instead.
            // By the time this runs the document has been migrated.
            // So the only version it may carry is the current one.
            // GCOVR_EXCL_START
            schema["required"] = {
                "magic",
                "ticks",
                "state",
                "saying",
                "sayingTicksLeft",
                "hunger",
                "happiness",
                "meals",
                "disturbances",
                "pesters",
                "disturbed"};
            // GCOVR_EXCL_STOP
            schema["properties"]["magic"]["const"] =
                std::string(kSaveMagic);
            schema["properties"][std::string(replay::kSchemaVersionKey)]
                  ["const"] = kSaveFormatVersion;
            schema["properties"]["ticks"] = countShape();
            schema["properties"]["state"] = wordShape();
            schema["properties"]["saying"] = wordShape();
            schema["properties"]["sayingTicksLeft"] = countShape();
            schema["properties"]["hunger"] = countShape();
            schema["properties"]["happiness"] = countShape();
            schema["properties"]["meals"] = countShape();
            schema["properties"]["disturbances"] = countShape();
            schema["properties"]["pesters"] = countShape();
            schema["properties"]["disturbed"]["type"] = "boolean";
            return schema;
        }

        const nlohmann::json_schema::json_validator &petValidator()
        {
            static const nlohmann::json_schema::json_validator validator(
                petSchema()); // GCOVR_EXCL_LINE
            return validator;
        }
    } // namespace

    MigrationChain standardPetMigrations()
    {
        // The version key is the shared one, so none is passed.
        // Empty because this format has never been bumped.
        // The chain still reads the version and still stamps it.
        // So a document from a newer build is refused rather than read.
        return MigrationChain(
            antwika::replay::MigrationList{}, kSaveFormatVersion);
    }

    nlohmann::json petMemoryToJson(const PetMemory &memory)
    {
        nlohmann::json encoded;
        encoded["magic"] = std::string(kSaveMagic);
        encoded[std::string(replay::kSchemaVersionKey)] =
            kSaveFormatVersion;
        encoded["ticks"] = memory.ticks;
        encoded["state"] = std::string(stateName(memory.state));
        encoded["saying"] = std::string(sayingName(memory.saying));
        encoded["sayingTicksLeft"] = memory.sayingTicksLeft;
        encoded["hunger"] = memory.hunger;
        encoded["happiness"] = memory.happiness;
        encoded["meals"] = memory.meals;
        encoded["disturbances"] = memory.disturbances;
        encoded["pesters"] = memory.pesters;
        encoded["disturbed"] = memory.disturbed;
        return encoded;

        // gcov puts the cleanup block on this closing brace.
        // ReplayJson.cpp's own encoder explains it at length.
        // No input reaches it.
    } // GCOVR_EXCL_LINE

    PetMemory petMemoryFromJson(const nlohmann::json &document)
    {
        const auto migrated =
            replay::readVersionedDocument<SaveFormatError>(
                document,
                standardPetMigrations(),
                petValidator(),
                "antwika::companion: a saved companion failed schema "
                "validation: ");

        return PetMemory{
            .ticks = migrated.at("ticks").get<Tick>(),
            .state =
                stateFromName(migrated.at("state").get<std::string>()),
            .saying =
                sayingFromName(migrated.at("saying").get<std::string>()),
            .sayingTicksLeft =
                migrated.at("sayingTicksLeft").get<Tick>(),
            .hunger = migrated.at("hunger").get<std::uint32_t>(),
            .happiness = migrated.at("happiness").get<std::uint32_t>(),
            .meals = migrated.at("meals").get<std::uint32_t>(),
            .disturbances =
                migrated.at("disturbances").get<std::uint32_t>(),
            .pesters = migrated.at("pesters").get<std::uint32_t>(),
            .disturbed = migrated.at("disturbed").get<bool>()};
    } // GCOVR_EXCL_LINE

    void writePetMemory(const PetMemory &memory, std::ostream &out)
    {
        out << petMemoryToJson(memory).dump(kIndent) << '\n';
    }

    PetMemory readPetMemory(std::istream &in)
    {
        nlohmann::json document;
        try
        {
            in >> document;
        }
        catch (const nlohmann::json::exception &error) // GCOVR_EXCL_LINE
        {
            throw SaveFormatError(
                std::string("antwika::companion: a saved companion is "
                            "not valid JSON: ")
                + error.what());
        }

        return petMemoryFromJson(document);
    }

} // namespace antwika::companion
