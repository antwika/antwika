#include "antwika/companion/ConfigFile.hpp"

#include <cstdint>
#include <limits>
#include <string>

#include <nlohmann/json-schema.hpp>

#include <antwika/config/ConfigDocument.hpp>
#include <antwika/config/Format.hpp>

namespace antwika::companion
{

    namespace
    {
        constexpr antwika::config::Format kFormat{
            .magic = kConfigMagic, .version = kConfigFormatVersion};

        // A period of no ticks would never come due.
        // So the floor is one tick, stated beside the parse.
        nlohmann::json period()
        {
            return antwika::config::wholeShape(
                1, std::numeric_limits<std::int64_t>::max());
        }

        // A gauge's ceiling has to hold at least one unit.
        nlohmann::json amount1()
        {
            return antwika::config::wholeShape(
                1, std::numeric_limits<std::uint32_t>::max());
        }

        // An amount a verb moves a gauge by may be nothing at all.
        nlohmann::json amount()
        {
            return antwika::config::wholeShape(
                0, std::numeric_limits<std::uint32_t>::max());
        }

        nlohmann::json configSchema()
        {
            auto schema = antwika::config::documentSchema(
                kFormat, "antwika companion config document");
            schema["properties"]["hungerPeriodTicks"] = period();
            schema["properties"]["starvePeriodTicks"] = period();
            schema["properties"]["funDecayPeriodTicks"] = period();
            schema["properties"]["fretPeriodTicks"] = period();
            schema["properties"]["recoverPeriodTicks"] = period();
            schema["properties"]["restPeriodTicks"] = period();
            schema["properties"]["sayingTicks"] = period();
            schema["properties"]["chatterPeriodTicks"] = period();
            schema["properties"]["drainHappyTicks"] = period();
            schema["properties"]["drainContentTicks"] = period();
            schema["properties"]["drainLowTicks"] = period();
            schema["properties"]["drainMiserableTicks"] = period();
            schema["properties"]["childTicks"] = period();
            schema["properties"]["teenTicks"] = period();
            schema["properties"]["adultTicks"] = period();
            schema["properties"]["elderTicks"] = period();
            schema["properties"]["hungerMax"] = amount1();
            schema["properties"]["funMax"] = amount1();
            schema["properties"]["happinessMax"] = amount1();
            schema["properties"]["hungerThreshold"] = amount();
            schema["properties"]["feedRelief"] = amount();
            schema["properties"]["feedJoy"] = amount();
            schema["properties"]["funStart"] = amount();
            schema["properties"]["playFun"] = amount();
            schema["properties"]["playHunger"] = amount();
            schema["properties"]["playEnergy"] = amount();
            schema["properties"]["playJoy"] = amount();
            schema["properties"]["happinessStart"] = amount();
            schema["properties"]["happyBand"] = amount();
            schema["properties"]["contentBand"] = amount();
            schema["properties"]["disturbCost"] = amount();
            schema["properties"]["pesterCost"] = amount();
            schema["properties"]["tiredPercent"] =
                antwika::config::wholeShape(0, 100);
            return schema;
        } // GCOVR_EXCL_LINE

        const nlohmann::json_schema::json_validator &configValidator()
        {
            static const nlohmann::json_schema::json_validator validator(
                configSchema()); // GCOVR_EXCL_LINE
            return validator;
        }
    } // namespace

    MigrationChain standardConfigMigrations()
    {
        // Every branch left on the excluded line is the allocator's.
        // The list is empty, so no heap branch is taken here.
        // What is left is the throw edge of building it.
        return MigrationChain({}, kConfigFormatVersion); // GCOVR_EXCL_LINE
    }

    nlohmann::json configToJson(const PetConfig &config)
    {
        auto encoded = antwika::config::newDocument(kFormat);
        encoded["hungerPeriodTicks"] = config.hungerPeriodTicks;
        encoded["starvePeriodTicks"] = config.starvePeriodTicks;
        encoded["funDecayPeriodTicks"] = config.funDecayPeriodTicks;
        encoded["fretPeriodTicks"] = config.fretPeriodTicks;
        encoded["recoverPeriodTicks"] = config.recoverPeriodTicks;
        encoded["restPeriodTicks"] = config.restPeriodTicks;
        encoded["sayingTicks"] = config.sayingTicks;
        encoded["chatterPeriodTicks"] = config.chatterPeriodTicks;
        encoded["drainHappyTicks"] = config.drainHappyTicks;
        encoded["drainContentTicks"] = config.drainContentTicks;
        encoded["drainLowTicks"] = config.drainLowTicks;
        encoded["drainMiserableTicks"] = config.drainMiserableTicks;
        encoded["childTicks"] = config.childTicks;
        encoded["teenTicks"] = config.teenTicks;
        encoded["adultTicks"] = config.adultTicks;
        encoded["elderTicks"] = config.elderTicks;
        encoded["hungerMax"] = config.hungerMax;
        encoded["funMax"] = config.funMax;
        encoded["happinessMax"] = config.happinessMax;
        encoded["hungerThreshold"] = config.hungerThreshold;
        encoded["feedRelief"] = config.feedRelief;
        encoded["feedJoy"] = config.feedJoy;
        encoded["funStart"] = config.funStart;
        encoded["playFun"] = config.playFun;
        encoded["playHunger"] = config.playHunger;
        encoded["playEnergy"] = config.playEnergy;
        encoded["playJoy"] = config.playJoy;
        encoded["happinessStart"] = config.happinessStart;
        encoded["happyBand"] = config.happyBand;
        encoded["contentBand"] = config.contentBand;
        encoded["disturbCost"] = config.disturbCost;
        encoded["pesterCost"] = config.pesterCost;
        encoded["tiredPercent"] = config.tiredPercent;
        return encoded;

        // gcov puts the cleanup block on this closing brace.
        // SaveGame.cpp's own encoder explains it at length.
        // No input reaches it.
    } // GCOVR_EXCL_LINE

    PetConfig configFromJson(const nlohmann::json &document)
    {
        using antwika::config::memberOr;

        const auto brought = antwika::config::migrated(
            document,
            standardConfigMigrations(),
            configValidator(),
            "antwika::companion: config JSON failed schema "
            "validation: ");

        PetConfig config;
        config.hungerPeriodTicks =
            memberOr(brought, "hungerPeriodTicks", config.hungerPeriodTicks);
        config.starvePeriodTicks =
            memberOr(brought, "starvePeriodTicks", config.starvePeriodTicks);
        config.funDecayPeriodTicks = memberOr(
            brought, "funDecayPeriodTicks", config.funDecayPeriodTicks);
        config.fretPeriodTicks =
            memberOr(brought, "fretPeriodTicks", config.fretPeriodTicks);
        config.recoverPeriodTicks =
            memberOr(brought, "recoverPeriodTicks", config.recoverPeriodTicks);
        config.restPeriodTicks =
            memberOr(brought, "restPeriodTicks", config.restPeriodTicks);
        config.sayingTicks =
            memberOr(brought, "sayingTicks", config.sayingTicks);
        config.chatterPeriodTicks = memberOr(
            brought, "chatterPeriodTicks", config.chatterPeriodTicks);
        config.drainHappyTicks =
            memberOr(brought, "drainHappyTicks", config.drainHappyTicks);
        config.drainContentTicks = memberOr(
            brought, "drainContentTicks", config.drainContentTicks);
        config.drainLowTicks =
            memberOr(brought, "drainLowTicks", config.drainLowTicks);
        config.drainMiserableTicks = memberOr(
            brought, "drainMiserableTicks", config.drainMiserableTicks);
        config.childTicks =
            memberOr(brought, "childTicks", config.childTicks);
        config.teenTicks =
            memberOr(brought, "teenTicks", config.teenTicks);
        config.adultTicks =
            memberOr(brought, "adultTicks", config.adultTicks);
        config.elderTicks =
            memberOr(brought, "elderTicks", config.elderTicks);
        config.hungerMax =
            memberOr(brought, "hungerMax", config.hungerMax);
        config.funMax =
            memberOr(brought, "funMax", config.funMax);
        config.happinessMax =
            memberOr(brought, "happinessMax", config.happinessMax);
        config.hungerThreshold =
            memberOr(brought, "hungerThreshold", config.hungerThreshold);
        config.feedRelief =
            memberOr(brought, "feedRelief", config.feedRelief);
        config.feedJoy =
            memberOr(brought, "feedJoy", config.feedJoy);
        config.funStart =
            memberOr(brought, "funStart", config.funStart);
        config.playFun =
            memberOr(brought, "playFun", config.playFun);
        config.playHunger =
            memberOr(brought, "playHunger", config.playHunger);
        config.playEnergy =
            memberOr(brought, "playEnergy", config.playEnergy);
        config.playJoy =
            memberOr(brought, "playJoy", config.playJoy);
        config.happinessStart =
            memberOr(brought, "happinessStart", config.happinessStart);
        config.happyBand =
            memberOr(brought, "happyBand", config.happyBand);
        config.contentBand =
            memberOr(brought, "contentBand", config.contentBand);
        config.disturbCost =
            memberOr(brought, "disturbCost", config.disturbCost);
        config.pesterCost =
            memberOr(brought, "pesterCost", config.pesterCost);
        config.tiredPercent =
            memberOr(brought, "tiredPercent", config.tiredPercent);
        return config;
    }

    void writeConfig(const PetConfig &config, std::ostream &out)
    {
        antwika::config::writeConfig(configToJson(config), out);
    }

    PetConfig readConfig(std::istream &in)
    {
        return configFromJson(antwika::config::parseConfig(in));
    }

    PetConfig loadConfigFileOrDefaults(const std::string &path)
    {
        const auto document = antwika::config::parseConfigFile(path);

        // A file that is not there is an install nobody has tuned.
        // Which is a state rather than a failure.
        return document.has_value() ? configFromJson(*document)
                                    : PetConfig{};
    }

} // namespace antwika::companion
