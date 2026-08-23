#pragma once

#include <cstddef>
#include <vector>

namespace antwika::wfc
{

    class CompatibilityTable final
    {
    public:
        explicit CompatibilityTable(std::size_t alphabetSize);

        void set(std::size_t left, std::size_t right, bool isCompatible);

        [[nodiscard]] bool compatible(
            std::size_t left, std::size_t right) const;

        [[nodiscard]] std::size_t getAlphabetSize() const;

    private:
        std::size_t size;
        std::vector<bool> entries;
    };

}
