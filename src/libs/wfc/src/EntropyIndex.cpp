#include "EntropyIndex.hpp"

#include <cmath>

namespace
{

    constexpr double kWeightedKeySteps = 1e9;

}

namespace antwika::wfc::detail
{

    EntropyIndex::EntropyIndex(
        const std::vector<Domain> &wave, std::vector<double> valueWeights)
        : valueWeights(std::move(valueWeights)), cellKey(wave.size())
    {
        for (std::size_t cell = 0; cell < wave.size(); ++cell)
        {
            update(cell, wave[cell]);
        }
    }

    double EntropyIndex::sortKey(const Domain &domain) const
    {
        if (valueWeights.empty())
        {
            return static_cast<double>(domain.count());
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

        if (domain.count() <= 1)
        {
            return;
        }

        const std::pair<double, std::size_t> key{sortKey(domain), cell};
        keysByEntropy.insert(key);
        cellKey[cell] = key;
    }

    std::optional<std::size_t> EntropyIndex::pickNext() const
    {
        if (keysByEntropy.empty())
        {
            return std::nullopt;
        }
        return keysByEntropy.begin()->second;
    }

}
