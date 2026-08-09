#pragma once

#include <vector>

#include "antwika/pattern/Controls.hpp"
#include "antwika/pattern/Cycle.hpp"
#include "antwika/pattern/Pattern.hpp"

namespace antwika::pattern
{

    [[nodiscard]] Pattern silence();

    [[nodiscard]] Pattern pure(Controls value);

    [[nodiscard]] Pattern steady(Controls value);

    [[nodiscard]] Pattern stack(std::vector<Pattern> layers);

    [[nodiscard]] Pattern slowcat(std::vector<Pattern> parts);

    [[nodiscard]] Pattern fastcat(std::vector<Pattern> parts);

    struct Slice final
    {
        Cycle weight{1};

        Pattern part = silence();
    };

    [[nodiscard]] Pattern timecat(std::vector<Slice> parts);

}
