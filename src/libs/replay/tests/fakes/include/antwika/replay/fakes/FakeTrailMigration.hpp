#pragma once

#include <nlohmann/json.hpp>

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

#include "antwika/replay/IMigration.hpp"

namespace antwika::replay::fakes
{

    class FakeTrailMigration final : public IMigration
    {
    public:
        FakeTrailMigration(
            std::uint32_t from, std::uint32_t to, std::string label)
            : from(from), to(to), label(std::move(label))
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
            return label;
        }

        void apply(nlohmann::json &document) const override
        {
            document["trail"].push_back(label);
        }

    private:
        std::uint32_t from;
        std::uint32_t to;
        std::string label;
    };

}
