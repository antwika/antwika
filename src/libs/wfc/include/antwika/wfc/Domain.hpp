#pragma once

#include <cstddef>
#include <vector>

namespace antwika::wfc
{

    class Domain final
    {
    public:
        class const_iterator final
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

        explicit Domain(std::size_t alphabetSize);

        [[nodiscard]] static Domain singleton(
            std::size_t value, std::size_t alphabetSize);

        [[nodiscard]] bool contains(std::size_t value) const;

        void remove(std::size_t value);

        void add(std::size_t value);

        void restrictTo(std::size_t value);

        [[nodiscard]] std::size_t alphabetSize() const;

        [[nodiscard]] std::size_t count() const;

        [[nodiscard]] bool isEmpty() const;

        [[nodiscard]] bool isSingleton() const;

        [[nodiscard]] std::size_t singleValue() const;

        [[nodiscard]] const_iterator begin() const;

        [[nodiscard]] const_iterator end() const;

        [[nodiscard]] bool operator==(const Domain &other) const;

    private:
        std::vector<bool> bits;

        std::size_t setCount;
    };

}
