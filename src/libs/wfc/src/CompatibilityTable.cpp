#include "antwika/wfc/CompatibilityTable.hpp"

namespace antwika::wfc
{

    CompatibilityTable::CompatibilityTable(std::size_t alphabetSize)
        : size(alphabetSize), entries(alphabetSize * alphabetSize, true)
    {
    }

    void CompatibilityTable::set(
        std::size_t left, std::size_t right, bool isCompatible)
    {
        if (left >= size || right >= size)
        {
            return;
        }
        entries[left * size + right] = isCompatible;
    }

    bool CompatibilityTable::compatible(
        std::size_t left, std::size_t right) const
    {
        if (left >= size || right >= size)
        {
            return false;
        }
        return entries[left * size + right];
    }

    std::size_t CompatibilityTable::getAlphabetSize() const
    {
        return size;
    }

}
