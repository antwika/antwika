#pragma once

#include <nlohmann/json_fwd.hpp>

#include <cstdint>
#include <string_view>

#include "antwika/replay/IMigration.hpp"

namespace antwika::replay::fakes
{

    class FakeCountMigration final : public IMigration
    {
    public:
        FakeCountMigration(std::uint32_t from, std::uint32_t to)
            : from(from), to(to)
        {
        }

        [[nodiscard]] std::uint32_t fromVersion() const noexcept override
        {
            return from;
        }

        [[nodiscard]] std::uint32_t toVersion() const noexcept override
        {
            return to;
        }

        [[nodiscard]] std::string_view name() const noexcept override
        {
            return "toy-add-count";
        }

        void apply(nlohmann::json &document) const override
        {
            document["count"] = 0;
        }

    private:
        std::uint32_t from;
        std::uint32_t to;
    };

}
