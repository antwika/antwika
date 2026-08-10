#pragma once

#include <nlohmann/json_fwd.hpp>

#include <cstdint>
#include <string_view>

namespace antwika::replay
{

    class IMigration
    {
    public:
        IMigration() = default;
        IMigration(const IMigration &) = delete;
        IMigration(IMigration &&) = delete;
        IMigration &operator=(const IMigration &) = delete;
        IMigration &operator=(IMigration &&) = delete;
        virtual ~IMigration() = default;

        [[nodiscard]] virtual std::uint32_t fromVersion() const
            noexcept = 0;

        [[nodiscard]] virtual std::uint32_t toVersion() const
            noexcept = 0;

        [[nodiscard]] virtual std::string_view name() const noexcept = 0;

        virtual void apply(nlohmann::json &document) const = 0;
    };

}
