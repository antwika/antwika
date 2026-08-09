#include "antwika/poker/ConfigFile.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>

#include <antwika/config/AppConfigFile.hpp>
#include <antwika/config/ConfigDocument.hpp>
#include <antwika/config/ConfigFormatError.hpp>

namespace antwika::poker
{

    namespace
    {
        using antwika::config::memberOr;
        using antwika::config::wholeShape;

        nlohmann::json chipsShape()
        {
            return wholeShape(
                1, std::numeric_limits<std::int64_t>::max());
        }

        void describeMembers(nlohmann::json &schema)
        {
            schema["properties"]["smallBlind"] = chipsShape();
            schema["properties"]["bigBlind"] = chipsShape();
            schema["properties"]["minimumBuyIn"] = chipsShape();

            auto &limits = schema["properties"]["thresholds"];
            limits["type"] = "array";
            limits["items"]["type"] = "object";
            limits["items"]["additionalProperties"] = false;
            limits["items"]["required"] = {
                "raiseAt", "callAt"}; // GCOVR_EXCL_LINE
            limits["items"]["properties"]["raiseAt"] =
                wholeShape(0, 100);
            limits["items"]["properties"]["callAt"] =
                wholeShape(0, 100);
            limits["minItems"] = kAgentStyleCount;
            limits["maxItems"] = kAgentStyleCount;

            auto &strengths = schema["properties"]["handStrengths"];
            strengths["type"] = "array";
            strengths["items"] = wholeShape(0, 100);
            strengths["minItems"] = kHandCategoryCount;
            strengths["maxItems"] = kHandCategoryCount;
        }

        void encodeMembers(const RoomConfig &config, nlohmann::json &out)
        {
            out["smallBlind"] = config.blinds.small;
            out["bigBlind"] = config.blinds.big;
            out["minimumBuyIn"] = config.minimumBuyIn;

            for (const auto limit : config.thresholds)
            {
                nlohmann::json one;
                one["raiseAt"] = limit.raiseAt;
                one["callAt"] = limit.callAt;
                out["thresholds"].push_back(std::move(one));
            }

            for (const auto strength : config.handStrengths)
            {
                out["handStrengths"].push_back(strength);
            }
        }

        RoomConfig decodeMembers(const nlohmann::json &document)
        {
            RoomConfig config;
            config.blinds.small =
                memberOr(document, "smallBlind", config.blinds.small);
            config.blinds.big =
                memberOr(document, "bigBlind", config.blinds.big);
            config.minimumBuyIn =
                memberOr(document, "minimumBuyIn", config.minimumBuyIn);

            if (document.contains("handStrengths"))
            {
                const auto &strengths = document.at("handStrengths");

                for (std::size_t index = 0;
                     index < kHandCategoryCount;
                     ++index)
                {
                    config.handStrengths[index] =
                        strengths.at(index).get<unsigned>();
                }
            }

            if (config.blinds.big < config.blinds.small)
            {
                throw antwika::config::ConfigFormatError(
                    "antwika::poker: config states a big blind "
                    "smaller than the small blind");
            }

            if (document.contains("thresholds"))
            {
                const auto &limits = document.at("thresholds");

                for (std::size_t index = 0; index < kAgentStyleCount;
                     ++index)
                {
                    config.thresholds[index] = AgentThresholds{
                        .raiseAt =
                            limits.at(index).at("raiseAt").get<unsigned>(),
                        .callAt =
                            limits.at(index).at("callAt").get<unsigned>()};
                }
            }

            for (const auto limit : config.thresholds)
            {
                if (limit.raiseAt < limit.callAt)
                {
                    throw antwika::config::ConfigFormatError(
                        "antwika::poker: config gives a style a raise "
                        "threshold under its call threshold");
                }
            }

            for (std::size_t index = 1; index < kHandCategoryCount;
                 ++index)
            {
                if (config.handStrengths[index]
                    < config.handStrengths[index - 1])
                {
                    throw antwika::config::ConfigFormatError(
                        "antwika::poker: config rates a stronger "
                        "hand category under a weaker one");
                }
            }

            if (config.minimumBuyIn < config.blinds.big)
            {
                throw antwika::config::ConfigFormatError(
                    "antwika::poker: config states a minimum "
                    "buy-in smaller than the big blind");
            }
            return config;
        }
    }

    ANTWIKA_CONFIG_FILE(
        "poker",
        RoomConfig,
        describeMembers,
        encodeMembers,
        decodeMembers)

}
