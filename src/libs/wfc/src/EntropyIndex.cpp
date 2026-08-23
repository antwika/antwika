#include "EntropyIndex.hpp"

#include <cmath>

namespace
{

    constexpr double kWeightedKeySteps = 1e9;

}

namespace antwika::wfc::detail
{

    EntropyIndex::EntropyIndex(
        const std::vector<Domain> &waveDomains,
        std::vector<double> valueWeights)
        : valueWeights(std::move(valueWeights)), cellKey(waveDomains.size())
    {
        for (std::size_t cell = 0; cell < waveDomains.size(); ++cell)
        {
            update(cell, waveDomains[cell]);
        }
    }

    double EntropyIndex::getSortKey(const Domain &domain) const
    {
        if (valueWeights.empty())
        {
            return static_cast<double>(domain.getCount());
        }

        double totalWeight = 0.0;
        double weightedLogSum = 0.0;
        for (const std::size_t value : domain)
        {
            const double weight = valueWeights[value];
            totalWeight += weight;
            weightedLogSum += weight * std::log(weight);
        }

        const double entropy =
            std::log(totalWeight) - weightedLogSum / totalWeight;
        return std::round(entropy * kWeightedKeySteps);
    }

    void EntropyIndex::update(std::size_t cell, const Domain &domain)
    {
        if (cellKey[cell].has_value())
        {
            keysByEntropy.erase(*cellKey[cell]);
            cellKey[cell].reset();
        }

        if (domain.getCount() <= 1)
        {
            return;
        }

        const std::pair<double, std::size_t> key{getSortKey(domain), cell};
        keysByEntropy.insert(key);
        cellKey[cell] = key;
    }

    std::optional<std::size_t> EntropyIndex::getPickNext() const
    {
        if (keysByEntropy.empty())
        {
            return std::nullopt;
        }
        return keysByEntropy.begin()->second;
    }

}
