#include "antwika/replay/SchemaVersion.hpp"

#include <format>
#include <limits>
#include <string>

#include <antwika/replay/SchemaVersionError.hpp>

namespace antwika::replay
{

    namespace
    {
        constexpr std::int64_t kMaxVersion =
            std::numeric_limits<std::uint32_t>::max();

        bool fitsAVersion(const nlohmann::json &stated)
        {
            if (stated.is_number_unsigned())
            {
                return stated.get<std::uint64_t>()
                       <= static_cast<std::uint64_t>(kMaxVersion);
            }
            if (stated.is_number_integer())
            {
                const auto value = stated.get<std::int64_t>();
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
            return kUnversionedDocumentVersion;
        }

        const nlohmann::json &stated = document.at(key);
        if (!fitsAVersion(stated))
        {
            std::string found;

            if (stated.is_primitive())
            {
                found = stated.dump();
            }
            else
            {
                found = std::string("a JSON ") + stated.type_name();
            }

            throw SchemaVersionError(std::format(
                "antwika::replay: \"{}\" is not a schema version this "
                "build can read: expected a whole number from 0 to {}, "
                "found {}",
                key,
                kMaxVersion,
                found));
        }
        return stated.get<std::uint32_t>();
    }

}
