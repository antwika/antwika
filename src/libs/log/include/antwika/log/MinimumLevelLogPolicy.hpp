#pragma once

#include "ILogPolicy.hpp"

namespace antwika::log
{

    class MinimumLevelLogPolicy final : public ILogPolicy
    {
    public:
        explicit MinimumLevelLogPolicy(Level minimumLevel);

        MinimumLevelLogPolicy(const MinimumLevelLogPolicy &) = delete;
        MinimumLevelLogPolicy(MinimumLevelLogPolicy &&) = delete;

        MinimumLevelLogPolicy &operator=(
            const MinimumLevelLogPolicy &) = delete;
        MinimumLevelLogPolicy &operator=(MinimumLevelLogPolicy &&) = delete;

        [[nodiscard]] bool accepts(Level level) const noexcept override;

    private:
        Level minimumLevel;
    };

}
