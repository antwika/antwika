#include "antwika/companion/PetSave.hpp"

#include <array>
#include <cstddef>
#include <memory>
#include <string>

#include <nlohmann/json-schema.hpp>

#include <antwika/replay/IMigration.hpp>
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

        constexpr std::array<std::string_view, 16> kSayingNames{
            "none",
            "hello",
            "bored",
            "niceDay",
            "silly",
            "feedMe",
            "yum",
            "notHungry",
            "letMeSleep",
            "zzz",
            "playWithMe",
            "wheee",
            "tooTired",
            "notSleepy",
            "yawn",
            "poked"};

        static_assert(
            kSayingNames.size()
            == static_cast<std::size_t>(Saying::Poked) + 1);

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

        // What a version 1 companion becomes.
        //
        // Version 1 described an animal that died of unhappiness.
        // This one lives on its energy instead.
        // So nothing honest carries a version 1 document's needs over.
        // What that build called half-starved means nothing here.
        // What survives is what still means the same thing.
        // How long it lived, what was done to it, and whether it lives.
        //
        // The numbers below are this build's shipped defaults.
        // A migration is handed a document and never a configuration.
        // And those defaults are the balance the document was written on.
        class PetV1ToV2 final : public antwika::replay::IMigration
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

            // MigrationChain asks for this in one place only.
            // It is the message thrown when a step is not one step.
            // This one reads 1 and produces 2.
            // So reaching it means editing the two functions above.
            // Which breaks the migration rather than feeding it input.
            // See docs/confirming-unreachable-branches.md.
            // GCOVR_EXCL_START
            [[nodiscard]] std::string_view name() const noexcept override
            {
                return "companion: happiness gives way to energy";
            }
            // GCOVR_EXCL_STOP

            void apply(nlohmann::json &document) const override
            {
                // Version 1 said perished when its happiness ran out.
                // Here that is the ceiling running out instead.
                // And the ceiling is arithmetic over the collapses.
                // So a perished one arrives with enough of them.
                const bool gone = document.value("state", std::string())
                                  == kStateNames.back();

                document["fun"] = kFunStart;
                document["energy"] = gone ? 0 : kEnergyBase;
                document["day"] = 0;
                document["plays"] = 0;
                document["collapses"] = gone ? kCollapsesToPerish : 0;
                document["woken"] = document.value("disturbed", false);
                document.erase("disturbed");
            }

            // The most any stage allows, in collapses, rounded up.
            // So this many is certainly nothing left.
            // Whatever age the document says the companion reached.
            static constexpr std::uint32_t kCollapsesToPerish =
                (kEnergyBase + 3 * kStageEnergyBonus + kCollapsePenalty
                 - 1)
                / kCollapsePenalty;
        };

        // What a version 2 companion becomes.
        // The first of its line, with no record behind it.
        // Which is what every companion written before one truly was.
        class PetV2ToV3 final : public antwika::replay::IMigration
        {
        public:
            [[nodiscard]] std::uint32_t fromVersion() const noexcept
                override
            {
                return 2;
            }

            [[nodiscard]] std::uint32_t toVersion() const noexcept
                override
            {
                return 3;
            }

            // Unreachable for the reason the one above gives.
            // GCOVR_EXCL_START
            [[nodiscard]] std::string_view name() const noexcept override
            {
                return "companion: the file starts keeping a lineage";
            }
            // GCOVR_EXCL_STOP

            void apply(nlohmann::json &document) const override
            {
                document["generation"] = 1;
                document["bestTicks"] = 0;
            }
        };

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
                "fun",
                "happiness",
                "energy",
                "day",
                "meals",
                "plays",
                "disturbances",
                "pesters",
                "collapses",
                "woken",
                "generation",
                "bestTicks"};
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
            schema["properties"]["fun"] = countShape();
            schema["properties"]["happiness"] = countShape();
            schema["properties"]["energy"] = countShape();
            schema["properties"]["day"] = countShape();
            schema["properties"]["meals"] = countShape();
            schema["properties"]["plays"] = countShape();
            schema["properties"]["disturbances"] = countShape();
            schema["properties"]["pesters"] = countShape();
            schema["properties"]["collapses"] = countShape();
            schema["properties"]["woken"]["type"] = "boolean";
            schema["properties"]["generation"] = countShape();
            schema["properties"]["bestTicks"] = countShape();
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
        antwika::replay::MigrationList migrations;
        migrations.push_back(std::make_shared<const PetV1ToV2>());
        migrations.push_back(std::make_shared<const PetV2ToV3>());

        return MigrationChain(std::move(migrations), kSaveFormatVersion);
    }

    nlohmann::json companionMemoryToJson(const CompanionMemory &memory)
    {
        nlohmann::json encoded;
        encoded["magic"] = std::string(kSaveMagic);
        encoded[std::string(replay::kSchemaVersionKey)] =
            kSaveFormatVersion;
        encoded["ticks"] = memory.pet.ticks;
        encoded["state"] = std::string(stateName(memory.pet.state));
        encoded["saying"] = std::string(sayingName(memory.pet.saying));
        encoded["sayingTicksLeft"] = memory.pet.sayingTicksLeft;
        encoded["hunger"] = memory.pet.hunger;
        encoded["fun"] = memory.pet.fun;
        encoded["happiness"] = memory.pet.happiness;
        encoded["energy"] = memory.pet.energy;
        encoded["day"] = memory.pet.day;
        encoded["meals"] = memory.pet.meals;
        encoded["plays"] = memory.pet.plays;
        encoded["disturbances"] = memory.pet.disturbances;
        encoded["pesters"] = memory.pet.pesters;
        encoded["collapses"] = memory.pet.collapses;
        encoded["woken"] = memory.pet.woken;
        encoded["generation"] = memory.lineage.generation;
        encoded["bestTicks"] = memory.lineage.bestTicks;
        return encoded;

        // gcov puts the cleanup block on this closing brace.
        // ReplayJson.cpp's own encoder explains it at length.
        // No input reaches it.
    } // GCOVR_EXCL_LINE

    CompanionMemory companionMemoryFromJson(
        const nlohmann::json &document)
    {
        const auto migrated =
            replay::readVersionedDocument<SaveFormatError>(
                document,
                standardPetMigrations(),
                petValidator(),
                "antwika::companion: a saved companion failed schema "
                "validation: ");

        return CompanionMemory{
            .pet =
                PetMemory{
                    .ticks = migrated.at("ticks").get<Tick>(),
                    .state = stateFromName(
                        migrated.at("state").get<std::string>()),
                    .saying = sayingFromName(
                        migrated.at("saying").get<std::string>()),
                    .sayingTicksLeft =
                        migrated.at("sayingTicksLeft").get<Tick>(),
                    .hunger =
                        migrated.at("hunger").get<std::uint32_t>(),
                    .fun = migrated.at("fun").get<std::uint32_t>(),
                    .happiness =
                        migrated.at("happiness").get<std::uint32_t>(),
                    .energy =
                        migrated.at("energy").get<std::uint32_t>(),
                    .day = migrated.at("day").get<std::uint32_t>(),
                    .meals = migrated.at("meals").get<std::uint32_t>(),
                    .plays = migrated.at("plays").get<std::uint32_t>(),
                    .disturbances =
                        migrated.at("disturbances").get<std::uint32_t>(),
                    .pesters =
                        migrated.at("pesters").get<std::uint32_t>(),
                    .collapses =
                        migrated.at("collapses").get<std::uint32_t>(),
                    .woken = migrated.at("woken").get<bool>()},
            .lineage = LineageMemory{
                .generation =
                    migrated.at("generation").get<std::uint32_t>(),
                .bestTicks = migrated.at("bestTicks").get<Tick>()}};
    } // GCOVR_EXCL_LINE

    void writeCompanionMemory(
        const CompanionMemory &memory, std::ostream &out)
    {
        out << companionMemoryToJson(memory).dump(kIndent) << '\n';
    }

    CompanionMemory readCompanionMemory(std::istream &in)
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

        return companionMemoryFromJson(document);
    }

} // namespace antwika::companion
