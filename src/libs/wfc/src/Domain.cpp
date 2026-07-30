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

    Domain::Domain(std::size_t alphabetSize) : bits(alphabetSize, true)
    {
    }

    // The closing brace below is flagged as a gcov miss.
    // Every statement in the body still executes on every call.
    // gcc emits a separate scope-exit block for `domain`'s destructor.
    // Named return value optimization elides that block.
    // Its counter is left at zero.
    // Not a reachable, untested branch -- a known gcov/NRVO artifact.
    // See docs/confirming-unreachable-branches.md.
    Domain Domain::singleton(std::size_t value, std::size_t alphabetSize)
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
        if (value >= bits.size())
        {
            return;
        }
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
        // This was an assert, and every documented build is Release.
        // So a non-singleton reached the caller as the value 0 instead.
        // Solver then reported an unsolved wave Solved.
        // That is the exact failure blog/008 was written about.
        if (!isSingleton())
        {
            throw WfcError(
                "Domain: singleValue() needs a singleton domain");
        }

        // The check above is what makes this search unable to fail.
        return static_cast<std::size_t>(std::distance(
            bits.begin(), std::find(bits.begin(), bits.end(), true)));
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
