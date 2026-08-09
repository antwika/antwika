#pragma once

#include <string_view>

#include <antwika/pattern/Pattern.hpp>

#include "antwika/notation/IWordReader.hpp"

namespace antwika::notation
{

    using antwika::pattern::Pattern;

    [[nodiscard]] Pattern parsePattern(
        std::string_view source, const IWordReader &words);

}
