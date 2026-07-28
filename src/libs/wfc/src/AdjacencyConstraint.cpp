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
        // Unreachable in practice.
        // Any value removed here had no compatible leftValue.
        // It had none in the original leftDomain either.
        // So it could not have kept leftDomain from emptying above.
        // A surviving leftValue is compatible with some rightDomain value.
        // That keeps it from being removed here, in the second pass.
        // So once leftDomain is non-empty here, rightDomain cannot too.
        // Kept as a defensive check for a future structure change.
        // See docs/confirming-unreachable-branches.md.
        if (rightDomain.isEmpty()) // GCOVR_EXCL_LINE
        {
            return false; // GCOVR_EXCL_LINE
        }

        return true;
    }

} // namespace antwika::wfc
