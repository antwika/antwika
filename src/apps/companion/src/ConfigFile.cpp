#include "antwika/companion/ConfigFile.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>

#include <antwika/config/AppConfigFile.hpp>
#include <antwika/config/ConfigDocument.hpp>

namespace antwika::companion
{

    namespace
    {
        using antwika::config::memberOr;
        using antwika::config::wholeShape;

        nlohmann::json period()
        {
            return wholeShape(
                1, std::numeric_limits<std::int64_t>::max());
        }

        nlohmann::json amount1()
        {
            return wholeShape(
                1, std::numeric_limits<std::uint32_t>::max());
        }

        nlohmann::json amount()
        {
            return wholeShape(
                0, std::numeric_limits<std::uint32_t>::max());
        }

        void describeMembers(nlohmann::json &schema)
        {
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
            schema["properties"]["tiredPercent"] = wholeShape(0, 100);
        }

        void encodeMembers(const PetConfig &config, nlohmann::json &out)
        {
            out["hungerPeriodTicks"] = config.hungerPeriodTicks;
            out["starvePeriodTicks"] = config.starvePeriodTicks;
            out["funDecayPeriodTicks"] = config.funDecayPeriodTicks;
            out["fretPeriodTicks"] = config.fretPeriodTicks;
            out["recoverPeriodTicks"] = config.recoverPeriodTicks;
            out["restPeriodTicks"] = config.restPeriodTicks;
            out["sayingTicks"] = config.sayingTicks;
            out["chatterPeriodTicks"] = config.chatterPeriodTicks;
            out["drainHappyTicks"] = config.drainHappyTicks;
            out["drainContentTicks"] = config.drainContentTicks;
            out["drainLowTicks"] = config.drainLowTicks;
            out["drainMiserableTicks"] = config.drainMiserableTicks;
            out["childTicks"] = config.childTicks;
            out["teenTicks"] = config.teenTicks;
            out["adultTicks"] = config.adultTicks;
            out["elderTicks"] = config.elderTicks;
            out["hungerMax"] = config.hungerMax;
            out["funMax"] = config.funMax;
            out["happinessMax"] = config.happinessMax;
            out["hungerThreshold"] = config.hungerThreshold;
            out["feedRelief"] = config.feedRelief;
            out["feedJoy"] = config.feedJoy;
            out["funStart"] = config.funStart;
            out["playFun"] = config.playFun;
            out["playHunger"] = config.playHunger;
            out["playEnergy"] = config.playEnergy;
            out["playJoy"] = config.playJoy;
            out["happinessStart"] = config.happinessStart;
            out["happyBand"] = config.happyBand;
            out["contentBand"] = config.contentBand;
            out["disturbCost"] = config.disturbCost;
            out["pesterCost"] = config.pesterCost;
            out["tiredPercent"] = config.tiredPercent;
        }

        PetConfig decodeMembers(const nlohmann::json &document)
        {
            PetConfig config;
            config.hungerPeriodTicks =
                memberOr(
                    document, "hungerPeriodTicks", config.hungerPeriodTicks);
            config.starvePeriodTicks =
                memberOr(
                    document, "starvePeriodTicks", config.starvePeriodTicks);
            config.funDecayPeriodTicks =
                memberOr(
                    document,
                    "funDecayPeriodTicks",
                    config.funDecayPeriodTicks);
            config.fretPeriodTicks =
                memberOr(document, "fretPeriodTicks", config.fretPeriodTicks);
            config.recoverPeriodTicks =
                memberOr(
                    document, "recoverPeriodTicks", config.recoverPeriodTicks);
            config.restPeriodTicks =
                memberOr(document, "restPeriodTicks", config.restPeriodTicks);
            config.sayingTicks =
                memberOr(document, "sayingTicks", config.sayingTicks);
            config.chatterPeriodTicks =
                memberOr(
                    document, "chatterPeriodTicks", config.chatterPeriodTicks);
            config.drainHappyTicks =
                memberOr(document, "drainHappyTicks", config.drainHappyTicks);
            config.drainContentTicks =
                memberOr(
                    document, "drainContentTicks", config.drainContentTicks);
            config.drainLowTicks =
                memberOr(document, "drainLowTicks", config.drainLowTicks);
            config.drainMiserableTicks =
                memberOr(
                    document,
                    "drainMiserableTicks",
                    config.drainMiserableTicks);
            config.childTicks =
                memberOr(document, "childTicks", config.childTicks);
            config.teenTicks =
                memberOr(document, "teenTicks", config.teenTicks);
            config.adultTicks =
                memberOr(document, "adultTicks", config.adultTicks);
            config.elderTicks =
                memberOr(document, "elderTicks", config.elderTicks);
            config.hungerMax =
                memberOr(document, "hungerMax", config.hungerMax);
            config.funMax =
                memberOr(document, "funMax", config.funMax);
            config.happinessMax =
                memberOr(document, "happinessMax", config.happinessMax);
            config.hungerThreshold =
                memberOr(document, "hungerThreshold", config.hungerThreshold);
            config.feedRelief =
                memberOr(document, "feedRelief", config.feedRelief);
            config.feedJoy =
                memberOr(document, "feedJoy", config.feedJoy);
            config.funStart =
                memberOr(document, "funStart", config.funStart);
            config.playFun =
                memberOr(document, "playFun", config.playFun);
            config.playHunger =
                memberOr(document, "playHunger", config.playHunger);
            config.playEnergy =
                memberOr(document, "playEnergy", config.playEnergy);
            config.playJoy =
                memberOr(document, "playJoy", config.playJoy);
            config.happinessStart =
                memberOr(document, "happinessStart", config.happinessStart);
            config.happyBand =
                memberOr(document, "happyBand", config.happyBand);
            config.contentBand =
                memberOr(document, "contentBand", config.contentBand);
            config.disturbCost =
                memberOr(document, "disturbCost", config.disturbCost);
            config.pesterCost =
                memberOr(document, "pesterCost", config.pesterCost);
            config.tiredPercent =
                memberOr(document, "tiredPercent", config.tiredPercent);
            return config;
        }
    }

    ANTWIKA_CONFIG_FILE(
        "companion",
        PetConfig,
        describeMembers,
        encodeMembers,
        decodeMembers)

}
