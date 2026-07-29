#pragma once

#include <cstddef>
#include <vector>

namespace antwika::wfc
{

    /**
     * @brief A square boolean matrix describing which symbol pairs may
     * sit next to each other across an AdjacencyConstraint.
     */
    class CompatibilityTable final
    {
    public:
        /**
         * @brief Construct a table with all pairs initially compatible.
         * @param alphabetSize Number of possible symbol values.
         */
        explicit CompatibilityTable(std::size_t alphabetSize);

        /**
         * @brief Mark a pair as compatible or incompatible.
         * @param left Symbol value on the left-hand cell.
         * @param right Symbol value on the right-hand cell.
         * @param isCompatible Whether left may sit next to right.
         */
        void set(std::size_t left, std::size_t right, bool isCompatible);

        /**
         * @brief Check whether a symbol pair is compatible.
         * @param left Symbol value on the left-hand cell.
         * @param right Symbol value on the right-hand cell.
         * @return True if left may sit next to right.
         */
        [[nodiscard]] bool compatible(
            std::size_t left, std::size_t right) const;

        /**
         * @brief Get the alphabet size this table was built for.
         * @return The alphabet size.
         */
        [[nodiscard]] std::size_t alphabetSize() const;

    private:
        std::size_t size;
        std::vector<bool> entries;
    };

} // namespace antwika::wfc
