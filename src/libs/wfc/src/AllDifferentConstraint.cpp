#include "antwika/wfc/AllDifferentConstraint.hpp"

namespace antwika::wfc
{

    AllDifferentConstraint::AllDifferentConstraint(
        std::vector<std::size_t> cellIndices)
        : cellIndices(std::move(cellIndices))
    {
    }

    std::span<const std::size_t> AllDifferentConstraint::cells() const
    {
        return cellIndices;
    }

    bool AllDifferentConstraint::prune(std::vector<Domain> &waveDomains) const
    {
        for (const std::size_t source : cellIndices)
        {
            const Domain &sourceDomain = waveDomains[source];
            if (!sourceDomain.isSingleton())
            {
                continue;
            }
            const std::size_t value = sourceDomain.getSingleValue();
            for (const std::size_t target : cellIndices)
            {
                if (target == source)
                {
                    continue;
                }
                Domain &targetDomain = waveDomains[target];
                targetDomain.remove(value);
                if (targetDomain.isEmpty())
                {
                    return false;
                }
            }
        }
        return true;
    }

}
