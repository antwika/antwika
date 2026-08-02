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

        // Removed as they are found, with no remove-list to defer it.
        // A left value's verdict reads rightDomain and never leftDomain.
        // So clearing a bit behind the cursor changes no later verdict.
        // The second pass is the mirror of that.
        // It reads only leftDomain, final by the time it starts.
        // This is the solver's hottest loop and now allocates nothing.
        // Scanning the alphabet, not the domain, is what allows that.
        const std::size_t leftAlphabet = leftDomain.alphabetSize();
        for (std::size_t leftValue = 0; leftValue < leftAlphabet;
             ++leftValue)
        {
            if (!leftDomain.contains(leftValue))
            {
                continue;
            }

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
                leftDomain.remove(leftValue);
            }
        }
        if (leftDomain.isEmpty())
        {
            return false;
        }

        const std::size_t rightAlphabet = rightDomain.alphabetSize();
        for (std::size_t rightValue = 0; rightValue < rightAlphabet;
             ++rightValue)
        {
            if (!rightDomain.contains(rightValue))
            {
                continue;
            }

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
                rightDomain.remove(rightValue);
            }
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
