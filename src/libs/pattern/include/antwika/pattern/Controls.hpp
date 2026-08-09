#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include "antwika/pattern/ParamId.hpp"
#include "antwika/pattern/ParamValue.hpp"

namespace antwika::pattern
{

    struct Control final
    {
        ParamId id = kNoParam;
        ParamValue value;

        [[nodiscard]] bool operator==(const Control &other) const
            = default;
    };

    class Controls final
    {
    public:
        Controls() = default;

        Controls(ParamId id, ParamValue value);

        void set(ParamId id, ParamValue value);

        [[nodiscard]] std::optional<ParamValue> get(ParamId id) const;

        [[nodiscard]] std::size_t size() const noexcept;

        [[nodiscard]] bool empty() const noexcept;

        [[nodiscard]] const std::vector<Control> &all() const noexcept;

        [[nodiscard]] Controls mergedWith(const Controls &over) const;

        [[nodiscard]] bool operator==(const Controls &other) const
            = default;

    private:
        std::vector<Control> entries;
    };

}
