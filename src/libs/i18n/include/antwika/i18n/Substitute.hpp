#pragma once

#include <span>
#include <string>
#include <string_view>

namespace antwika::i18n
{

    [[nodiscard]] std::string substitute(
        std::string_view pattern, std::span<const std::string_view> args);

}
