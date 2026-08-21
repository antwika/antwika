#include "antwika/schema/SchemaVersion.hpp"

#include <nlohmann/json.hpp>

#include <format>
#include <limits>
#include <string>

#include <antwika/schema/SchemaVersionError.hpp>

namespace antwika::schema
{

    namespace
    {
        constexpr std::int64_t kMaxVersion =
            std::numeric_limits<std::uint32_t>::max();

        bool fitsAVersion(const nlohmann::json &statedJson)
        {
            if (statedJson.is_number_unsigned())
            {
                return statedJson.get<std::uint64_t>()
                       <= static_cast<std::uint64_t>(kMaxVersion);
            }
            if (statedJson.is_number_integer())
            {
                const auto value = statedJson.get<std::int64_t>();
                return value >= 0 && value <= kMaxVersion;
            }
            return false;
        }
    }

    std::uint32_t documentVersion(
        const nlohmann::json &document, std::string_view versionKey)
    {
        const std::string key(versionKey);
        if (!document.is_object() || !document.contains(key))
        {
            return kImplicitDocumentVersion;
        }

        const nlohmann::json &statedJson = document.at(key);
        if (!fitsAVersion(statedJson))
        {
            std::string statedText;

            if (statedJson.is_primitive())
            {
                statedText = statedJson.dump();
            }
            else
            {
                statedText = std::string("a JSON ") + statedJson.type_name();
            }

            throw SchemaVersionError(std::format(
                "antwika::schema: \"{}\" is not a schema version this "
                "build can read: expected a whole number from 0 to {}, "
                "found {}",
                key,
                kMaxVersion,
                statedText));
        }
        return statedJson.get<std::uint32_t>();
    }

}
