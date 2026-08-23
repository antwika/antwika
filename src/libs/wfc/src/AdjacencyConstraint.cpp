#include "antwika/wfc/AdjacencyConstraint.hpp"

#include <utility>

#include "antwika/wfc/WfcError.hpp"

namespace antwika::wfc
{

    AdjacencyConstraint::AdjacencyConstraint(
        std::size_t left, std::size_t right, CompatibilityTable table)
        : AdjacencyConstraint(
              left,
              right,
              std::make_shared<const CompatibilityTable>(std::move(table)))
    {
    }

    AdjacencyConstraint::AdjacencyConstraint(
        std::size_t left,
        std::size_t right,
        std::shared_ptr<const CompatibilityTable> table)
        : cellIndices{left, right}, table(std::move(table))
    {
        if (this->table == nullptr)
        {
            throw WfcError("AdjacencyConstraint: the table is nothing");
        }
    }

    std::span<const std::size_t> AdjacencyConstraint::cells() const
    {
        return cellIndices;
    }

    bool AdjacencyConstraint::prune(std::vector<Domain> &waveDomains) const
    {
        const std::size_t left = cellIndices[0];
        const std::size_t right = cellIndices[1];

        Domain &leftDomain = waveDomains[left];
        Domain &rightDomain = waveDomains[right];

        const std::size_t leftAlphabet = leftDomain.getAlphabetSize();
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
                if (table->compatible(leftValue, rightValue))
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

        const std::size_t rightAlphabet = rightDomain.getAlphabetSize();
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
                if (table->compatible(leftValue, rightValue))
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
        if (rightDomain.isEmpty()) // GCOVR_EXCL_LINE
        {
            return false; // GCOVR_EXCL_LINE
        }

        return true;
    }

}
