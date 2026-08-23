#pragma once

#include <nlohmann/json_fwd.hpp>

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include <antwika/schema/IMigration.hpp>

namespace antwika::schema
{

    using Apply = std::function<void(nlohmann::json &)>;

    [[nodiscard]] std::shared_ptr<const IMigration> getStep(
        std::uint32_t fromVersion,
        std::uint32_t toVersion,
        std::string name,
        Apply apply);

}
