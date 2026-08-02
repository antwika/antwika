#include "antwika/pattern/HapBuffer.hpp"

#include <cstddef>
#include <vector>

#include "antwika/pattern/Hap.hpp"

namespace antwika::pattern
{

    void HapBuffer::accept(const Hap &hap)
    {
        collected.push_back(hap);
    }

    const std::vector<Hap> &HapBuffer::haps() const noexcept
    {
        return collected;
    }

    void HapBuffer::clear() noexcept
    {
        collected.clear();
    }

    void HapBuffer::reserve(std::size_t count)
    {
        collected.reserve(count);
    }

} // namespace antwika::pattern
