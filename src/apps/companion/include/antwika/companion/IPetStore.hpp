#pragma once

#include <optional>

#include "antwika/companion/CompanionMemory.hpp"

namespace antwika::companion
{

    class IPetStore
    {
    public:
        virtual ~IPetStore() = default;

        [[nodiscard]] virtual std::optional<CompanionMemory> load() = 0;

        virtual void save(const CompanionMemory &memory) = 0;
    };

}
