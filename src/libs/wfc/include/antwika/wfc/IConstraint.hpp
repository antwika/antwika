#pragma once

#include <cstddef>
#include <span>
#include <vector>

#include "antwika/wfc/Domain.hpp"

namespace antwika::wfc
{

    class IConstraint
    {
    public:
        virtual ~IConstraint() = default;

        [[nodiscard]] virtual std::span<const std::size_t> cells() const = 0;

        virtual bool prune(std::vector<Domain> &waveDomains) const = 0;
    };

}
