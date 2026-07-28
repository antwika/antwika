#include "antwika/wfc/Domain.hpp"

#include <cassert>

namespace antwika::wfc
{

    Domain::const_iterator::const_iterator(
        const std::vector<bool> *bits, std::size_t pos)
        : bits(bits), pos(pos)
    {
        skipToNextSet();
    }

    void Domain::const_iterator::skipToNextSet()
    {
        while (pos < bits->size() && !(*bits)[pos])
        {
            ++pos;
        }
    }

    std::size_t Domain::const_iterator::operator*() const
    {
        return pos;
    }

    Domain::const_iterator &Domain::const_iterator::operator++()
    {
        ++pos;
        skipToNextSet();
        return *this;
    }

    Domain::const_iterator Domain::const_iterator::operator++(int)
    {
        const_iterator copy = *this;
        ++(*this);
        return copy;
    }

    Domain::Domain(std::size_t alphabetSize) : bits(alphabetSize, true)
    {
    }

    Domain Domain::singleton(std::size_t value, std::size_t alphabetSize)
    {
        Domain domain(alphabetSize);
        domain.restrictTo(value);
        return domain;
    }

    bool Domain::contains(std::size_t value) const
    {
        return value < bits.size() && bits[value];
    }

    void Domain::remove(std::size_t value)
    {
        if (value < bits.size())
        {
            bits[value] = false;
        }
    }

    void Domain::add(std::size_t value)
    {
        if (value < bits.size())
        {
            bits[value] = true;
        }
    }

    void Domain::restrictTo(std::size_t value)
    {
        for (std::size_t i = 0; i < bits.size(); ++i)
        {
            bits[i] = (i == value);
        }
    }

    std::size_t Domain::alphabetSize() const
    {
        return bits.size();
    }

    std::size_t Domain::count() const
    {
        std::size_t total = 0;
        for (const bool bit : bits)
        {
            if (bit)
            {
                ++total;
            }
        }
        return total;
    }

    bool Domain::isEmpty() const
    {
        return count() == 0;
    }

    bool Domain::isSingleton() const
    {
        return count() == 1;
    }

    std::size_t Domain::singleValue() const
    {
        assert(isSingleton());
        for (std::size_t i = 0; i < bits.size(); ++i)
        {
            if (bits[i])
            {
                return i;
            }
        }
        assert(false);
        return 0;
    }

    Domain::const_iterator Domain::begin() const
    {
        return const_iterator(&bits, 0);
    }

    Domain::const_iterator Domain::end() const
    {
        return const_iterator(&bits, bits.size());
    }

} // namespace antwika::wfc
