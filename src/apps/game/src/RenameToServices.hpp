#pragma once

#include <nlohmann/json.hpp>

#include <cstdint>
#include <string_view>

#include <antwika/replay/IMigration.hpp>

namespace antwika::game
{

    class RenameToServices final : public antwika::replay::IMigration
    {
    public:
        [[nodiscard]] std::uint32_t fromVersion() const noexcept override;

        [[nodiscard]] std::uint32_t toVersion() const noexcept override;

        [[nodiscard]] std::string_view name() const noexcept override;

        void apply(nlohmann::json &document) const override;
    };

}
