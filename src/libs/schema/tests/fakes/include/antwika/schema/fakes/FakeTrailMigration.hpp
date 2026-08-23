#pragma once

#include <nlohmann/json.hpp>

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

#include "antwika/schema/IMigration.hpp"

namespace antwika::schema::fakes
{

    class FakeTrailMigration final : public IMigration
    {
    public:
        FakeTrailMigration(
            std::uint32_t startVersion, std::uint32_t endVersion,
            std::string label)
            : startVersion(startVersion), endVersion(endVersion),
                label(std::move(label))
        {
        }

        [[nodiscard]] std::uint32_t getFromVersion() const noexcept override
        {
            return startVersion;
        }

        [[nodiscard]] std::uint32_t toVersion() const noexcept override
        {
            return endVersion;
        }

        [[nodiscard]] std::string_view getName() const noexcept override
        {
            return label;
        }

        void apply(nlohmann::json &document) const override
        {
            document["trail"].push_back(label);
        }

    private:
        std::uint32_t startVersion;
        std::uint32_t endVersion;
        std::string label;
    };

}
