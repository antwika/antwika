#pragma once

#include <cstddef>
#include <utility>
#include <vector>

#include "antwika/wfc/Domain.hpp"
#include "EntropyIndex.hpp"

namespace antwika::wfc::detail
{

    using antwika::wfc::Domain;

    class Trail final
    {
    public:
        void record(std::size_t cell, std::size_t value);

        [[nodiscard]] std::size_t checkpoint() const;

        void rewindTo(
            std::size_t checkpoint,
            std::vector<Domain> &wave,
            EntropyIndex &entropyIndex);

    private:
        std::vector<std::pair<std::size_t, std::size_t>> entries;
    };

}
