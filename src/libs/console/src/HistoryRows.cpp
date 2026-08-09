#include "antwika/console/HistoryRows.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace antwika::console
{

    namespace
    {
        struct Cut final
        {
            std::size_t kept = 0;

            std::size_t dropped = 0;
        };

        [[nodiscard]] Cut cutFor(
            std::string_view text, std::size_t width) noexcept
        {
            const auto space = text.substr(0, width + 1).find_last_of(' ');

            if (space == std::string_view::npos || space == 0)
            {
                return Cut{.kept = width, .dropped = 0};
            }

            return Cut{.kept = space, .dropped = 1};
        }
    }

    std::vector<std::string> wrappedRows(
        std::string_view text, std::size_t columns)
    {
        const auto width = std::max<std::size_t>(columns, 1);

        std::vector<std::string> rows;

        while (text.size() > width)
        {
            const auto cut = cutFor(text, width);

            rows.emplace_back(text.substr(0, cut.kept));
            text.remove_prefix(cut.kept + cut.dropped);
        }

        rows.emplace_back(text);

        return rows;
    } // GCOVR_EXCL_LINE

    std::vector<std::string> historyRows(
        std::span<const std::string> lines,
        std::size_t columns,
        std::size_t shown)
    {
        std::vector<std::string> rows;

        for (const auto &line : lines)
        {
            for (auto &row : wrappedRows(line, columns))
            {
                rows.push_back(std::move(row));
            }
        }

        if (rows.size() > shown)
        {
            rows.erase(
                rows.begin(),
                std::next(
                    rows.begin(),
                    static_cast<std::ptrdiff_t>(rows.size() - shown)));
        }

        return rows;
    } // GCOVR_EXCL_LINE

}
