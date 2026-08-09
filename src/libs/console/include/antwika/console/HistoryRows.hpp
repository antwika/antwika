#pragma once

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace antwika::console
{

    /**
     * @brief Splits one line into rows no wider than a column budget.
     *
     * @param text The line to split, broken at spaces where it can be.
     * @param columns The widest row in characters, where zero means one.
     * @return The rows in order, one of them empty when text is empty.
     */
    [[nodiscard]] std::vector<std::string> wrappedRows(
        std::string_view text, std::size_t columns);

    /**
     * @brief Wraps history lines and keeps the newest rows of them.
     *
     * @param lines The history, oldest first.
     * @param columns The widest row in characters, where zero means one.
     * @param shown The most rows to keep.
     * @return The newest rows in order, never more than shown of them.
     */
    [[nodiscard]] std::vector<std::string> historyRows(
        std::span<const std::string> lines,
        std::size_t columns,
        std::size_t shown);

}
