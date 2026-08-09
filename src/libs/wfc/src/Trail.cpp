#include "Trail.hpp"

namespace antwika::wfc::detail
{

    void Trail::record(std::size_t cell, std::size_t value)
    {
        entries.emplace_back(cell, value);
    }

    std::size_t Trail::checkpoint() const
    {
        return entries.size();
    }

    void Trail::rewindTo(
        std::size_t checkpoint,
        std::vector<Domain> &wave,
        EntropyIndex &entropyIndex)
    {
        while (entries.size() > checkpoint)
        {
            const auto [cell, value] = entries.back();
            entries.pop_back();
            wave[cell].add(value);
            entropyIndex.update(cell, wave[cell]);
        }
    }

}
