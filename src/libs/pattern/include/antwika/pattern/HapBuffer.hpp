#pragma once

#include <cstddef>
#include <vector>

#include "antwika/pattern/Hap.hpp"
#include "antwika/pattern/IHapSink.hpp"

namespace antwika::pattern
{

    class HapBuffer final : public IHapSink
    {
    public:
        void accept(const Hap &hap) override;

        [[nodiscard]] const std::vector<Hap> &haps() const noexcept;

        void clear() noexcept;

        void reserve(std::size_t count);

    private:
        std::vector<Hap> collected;
    };

}
