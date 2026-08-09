#include "antwika/companion/PetSave.hpp"

#include <cstddef>
#include <memory>
#include <string>

#include <antwika/config/ConfigDocument.hpp>
#include <antwika/replay/IMigration.hpp>
#include <antwika/replay/JsonShapes.hpp>
#include <antwika/enums/FromName.hpp>
#include <antwika/replay/SchemaVersion.hpp>
#include <antwika/replay/VersionedDocument.hpp>

#include "antwika/companion/SaveFormatError.hpp"

namespace antwika::companion
{

    namespace
    {

        constexpr antwika::enums::NameTable<PetState> kStates{
            {"awake", "asleep", "perished"}};

        static_assert(
            kStates.names.size()
            == static_cast<std::size_t>(PetState::Perished) + 1);

        constexpr antwika::enums::NameTable<Saying> kSayings{
            {"none",
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
             "poked"}};

        static_assert(
            kSayings.names.size()
            == static_cast<std::size_t>(Saying::Poked) + 1);

        [[nodiscard]] PetState stateFromName(const std::string &name)
        {
            return antwika::enums::fromName<SaveFormatError>(
                kStates,
                name,
                "antwika::companion: a saved companion names a state "
                "that is not one of the three: ");
        }

        [[nodiscard]] Saying sayingFromName(const std::string &name)
        {
            return antwika::enums::fromName<SaveFormatError>(
                kSayings,
                name,
                "antwika::companion: a saved companion says a line "
                "this build does not have: ");
        }

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

            // GCOVR_EXCL_START
            [[nodiscard]] std::string_view name() const noexcept override
            {
                return "companion: happiness gives way to energy";
            }
            // GCOVR_EXCL_STOP

            void apply(nlohmann::json &document) const override
            {
                const bool gone = document.value("state", std::string())
                                  == kStates.names.back();

                document["fun"] = kFunStart;
                document["energy"] = gone ? 0 : kEnergyBase;
                document["day"] = 0;
                document["plays"] = 0;
                document["collapses"] = gone ? kCollapsesToPerish : 0;
                document["woken"] = document.value("disturbed", false);
                document.erase("disturbed");
            }

            static constexpr std::uint32_t kCollapsesToPerish =
                (kEnergyBase + 3 * kStageEnergyBonus + kCollapsePenalty
                 - 1)
                / kCollapsePenalty;
        };

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

        using replay::countShape;
        using replay::wordShape;

        nlohmann::json petSchema()
        {
            nlohmann::json schema = replay::documentShape(
                "antwika companion document",
                {"magic",
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
                 "bestTicks"});
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
        } // GCOVR_EXCL_LINE
    }

    MigrationChain standardPetMigrations()
    {
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
        encoded["state"] = std::string(kStates.name(memory.pet.state));
        encoded["saying"] = std::string(kSayings.name(memory.pet.saying));
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

    } // GCOVR_EXCL_LINE

    CompanionMemory companionMemoryFromJson(
        const nlohmann::json &document)
    {
        const auto migrated =
            antwika::config::migratedAs<SaveFormatError>(
                document,
                standardPetMigrations(),
                replay::validatorFor<petSchema>(),
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
        antwika::config::writeConfig(
            companionMemoryToJson(memory), out);
    }

    CompanionMemory readCompanionMemory(std::istream &in)
    {
        return companionMemoryFromJson(
            antwika::config::parseAs<SaveFormatError>(in));
    }

}
