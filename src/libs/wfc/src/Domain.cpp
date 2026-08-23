#include "antwika/wfc/Domain.hpp"

#include <algorithm>
#include <iterator>

#include "antwika/wfc/WfcError.hpp"

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

    Domain::Domain(std::size_t alphabetSize)
        : bits(alphabetSize, true), setCount(alphabetSize)
    {
    }

    Domain Domain::createSingleton(std::size_t value, std::size_t alphabetSize)
    {
        Domain domain(alphabetSize);
        domain.restrictTo(value);
        return domain;
    } // GCOVR_EXCL_LINE

    bool Domain::contains(std::size_t value) const
    {
        return value < bits.size() && bits[value];
    }

    void Domain::remove(std::size_t value)
    {
        if (!contains(value))
        {
            return;
        }

        bits[value] = false;
        --setCount;
    }

    void Domain::add(std::size_t value)
    {
        if (value >= bits.size() || contains(value))
        {
            return;
        }

        bits[value] = true;
        ++setCount;
    }

    void Domain::restrictTo(std::size_t value)
    {
        if (value >= bits.size())
        {
            return;
        }
        for (std::size_t i = 0; i < bits.size(); ++i)
        {
            bits[i] = (i == value);
        }
        setCount = 1;
    }

    std::size_t Domain::getAlphabetSize() const
    {
        return bits.size();
    }

    std::size_t Domain::getCount() const
    {
        return setCount;
    }

    bool Domain::isEmpty() const
    {
        return getCount() == 0;
    }

    bool Domain::isSingleton() const
    {
        return getCount() == 1;
    }

    std::size_t Domain::getSingleValue() const
    {
        if (!isSingleton())
        {
            throw WfcError(
                "Domain: getSingleValue() needs a singleton domain");
        }

        return static_cast<std::size_t>(std::distance(
            bits.begin(), std::find(bits.begin(), bits.end(), true)));
    }

    bool Domain::operator==(const Domain &other) const
    {
        return bits == other.bits;
    }

    Domain::const_iterator Domain::begin() const
    {
        return const_iterator(&bits, 0);
    }

    Domain::const_iterator Domain::end() const
    {
        return const_iterator(&bits, bits.size());
    }

}
