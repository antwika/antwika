#include "antwika/wfc/AdjacencyConstraint.hpp"

namespace antwika::wfc
{

    AdjacencyConstraint::AdjacencyConstraint(
        std::size_t left, std::size_t right, CompatibilityTable table)
        : cellIndices{left, right}, table(std::move(table))
    {
    }

    std::span<const std::size_t> AdjacencyConstraint::cells() const
    {
        return cellIndices;
    }

    bool AdjacencyConstraint::prune(std::vector<Domain> &wave) const
    {
        const std::size_t left = cellIndices[0];
        const std::size_t right = cellIndices[1];

        Domain &leftDomain = wave[left];
        Domain &rightDomain = wave[right];

        std::vector<std::size_t> leftToRemove;
        for (const std::size_t leftValue : leftDomain)
        {
            bool hasCompatible = false;
            for (const std::size_t rightValue : rightDomain)
            {
                if (table.compatible(leftValue, rightValue))
                {
                    hasCompatible = true;
                    break;
                }
            }
            if (!hasCompatible)
            {
                leftToRemove.push_back(leftValue);
            }
        }
        for (const std::size_t value : leftToRemove)
        {
            leftDomain.remove(value);
        }
        if (leftDomain.isEmpty())
        {
            return false;
        }

        std::vector<std::size_t> rightToRemove;
        for (const std::size_t rightValue : rightDomain)
        {
            bool hasCompatible = false;
            for (const std::size_t leftValue : leftDomain)
            {
                if (table.compatible(leftValue, rightValue))
                {
                    hasCompatible = true;
                    break;
                }
            }
            if (!hasCompatible)
            {
                rightToRemove.push_back(rightValue);
            }
        }
        for (const std::size_t value : rightToRemove)
        {
            rightDomain.remove(value);
        }
        if (rightDomain.isEmpty())
        {
            return false;
        }

        return true;
    }

} // namespace antwika::wfc
