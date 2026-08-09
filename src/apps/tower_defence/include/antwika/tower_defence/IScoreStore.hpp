#pragma once

#include <optional>

#include "antwika/tower_defence/HighScore.hpp"

namespace antwika::tower_defence
{

    class IScoreStore
    {
    public:
        virtual ~IScoreStore() = default;

        [[nodiscard]] virtual std::optional<HighScore> load() = 0;

        virtual void save(const HighScore &score) = 0;
    };

}
