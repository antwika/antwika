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
        // Unreachable: any rightValue removed here had no compatible
        // leftValue in the ORIGINAL leftDomain either, so it could
        // never have kept leftDomain from being emptied above -- and
        // conversely, whichever leftValue survived the left-hand pass
        // did so *because* it is compatible with some value still in
        // rightDomain, which is exactly what keeps that value from
        // being removed here. So once leftDomain (checked above) is
        // non-empty, rightDomain provably cannot become empty in this
        // second pass. Kept only as a defensive check against a future
        // change to the two-pass structure above, per
        // docs/confirming-unreachable-branches.md.
        if (rightDomain.isEmpty()) // GCOVR_EXCL_LINE
        {
            return false; // GCOVR_EXCL_LINE
        }

        return true;
    }

} // namespace antwika::wfc
