#pragma once

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace antwika::io
{

    [[nodiscard]] std::vector<std::string> getFilteredBySuffix(
        std::span<const std::string> names,
        std::string_view suffix,
        std::size_t most);

    [[nodiscard]] std::string getWithSuffix(
        std::string_view name, std::string_view suffix);

}
