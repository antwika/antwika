#pragma once

#include <cstddef>
#include <vector>

namespace antwika::wfc
{

    /**
     * @brief A bitset-backed set of candidate symbol indices for one
     * cell, over the range [0, alphabetSize).
     *
     * std::vector<bool> rather than a fixed-width std::bitset<N> so the
     * same type serves both a 3-symbol tile strip and Sudoku's 9-symbol
     * alphabet without a template parameter.
     */
    class Domain
    {
    public:
        /**
         * @brief Forward iterator over a Domain's remaining values, in
         * ascending order.
         */
        class const_iterator
        {
        public:
            using value_type = std::size_t;
            using difference_type = std::ptrdiff_t;

            const_iterator(const std::vector<bool> *bits, std::size_t pos);

            [[nodiscard]] std::size_t operator*() const;
            const_iterator &operator++();
            const_iterator operator++(int);
            bool operator==(const const_iterator &) const = default;

        private:
            const std::vector<bool> *bits;
            std::size_t pos;

            void skipToNextSet();
        };

        /**
         * @brief Construct a domain with every candidate value present.
         * @param alphabetSize Number of possible symbol values.
         */
        explicit Domain(std::size_t alphabetSize);

        /**
         * @brief Construct a domain holding exactly one candidate value.
         * @param value The single value to hold.
         * @param alphabetSize Number of possible symbol values.
         * @return A Domain whose only set bit is value.
         */
        [[nodiscard]] static Domain singleton(
            std::size_t value, std::size_t alphabetSize);

        /**
         * @brief Check whether a value is still a candidate.
         * @param value The value to check.
         * @return True if value is still present in this domain.
         */
        [[nodiscard]] bool contains(std::size_t value) const;

        /**
         * @brief Remove a value from the candidate set.
         * @param value The value to remove.
         */
        void remove(std::size_t value);

        /**
         * @brief Restore a previously removed value.
         * @param value The value to restore.
         *
         * Only ever called by Trail to undo a prior remove() -- see
         * PLAN_WFC.md 3.2 and 3.9.
         */
        void add(std::size_t value);

        /**
         * @brief Restrict this domain to exactly one value.
         * @param value The only value that remains a candidate.
         */
        void restrictTo(std::size_t value);

        /**
         * @brief Get the alphabet size this domain was built for.
         * @return The total number of possible symbol values, whether
         * or not they remain candidates.
         */
        [[nodiscard]] std::size_t alphabetSize() const;

        /**
         * @brief Count remaining candidate values.
         * @return The number of values still present.
         */
        [[nodiscard]] std::size_t count() const;

        /**
         * @brief Check whether no candidate values remain.
         * @return True if count() == 0.
         */
        [[nodiscard]] bool isEmpty() const;

        /**
         * @brief Check whether exactly one candidate value remains.
         * @return True if count() == 1.
         */
        [[nodiscard]] bool isSingleton() const;

        /**
         * @brief Get the sole remaining candidate value.
         * @return The single remaining value.
         *
         * Precondition: isSingleton() is true.
         */
        [[nodiscard]] std::size_t singleValue() const;

        /**
         * @brief Begin ascending iteration over remaining values.
         * @return An iterator to the first remaining value's bit.
         */
        [[nodiscard]] const_iterator begin() const;

        /**
         * @brief End ascending iteration over remaining values.
         * @return An iterator past the last bit.
         */
        [[nodiscard]] const_iterator end() const;

        bool operator==(const Domain &) const = default;

    private:
        std::vector<bool> bits;
    };

} // namespace antwika::wfc
