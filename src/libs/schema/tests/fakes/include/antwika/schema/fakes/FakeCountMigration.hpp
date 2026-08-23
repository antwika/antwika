#pragma once

#include <nlohmann/json_fwd.hpp>

#include <cstdint>
#include <string_view>

#include "antwika/schema/IMigration.hpp"

namespace antwika::schema::fakes
{

    class FakeCountMigration final : public IMigration
    {
    public:
        FakeCountMigration(std::uint32_t startVersion, std::uint32_t endVersion)
            : startVersion(startVersion), endVersion(endVersion)
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
            return "toy-add-count";
        }

        void apply(nlohmann::json &document) const override
        {
            document["count"] = 0;
        }

    private:
        std::uint32_t startVersion;
        std::uint32_t endVersion;
    };

}
