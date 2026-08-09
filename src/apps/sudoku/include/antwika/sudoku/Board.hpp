#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace antwika::sudoku
{

    struct Square final
    {
        std::size_t row = 0;
        std::size_t col = 0;

        [[nodiscard]] bool operator==(const Square &other) const
            = default;
    };

    class Board final
    {
    public:
        static constexpr std::size_t kSize = 9;
        static constexpr std::size_t kCellCount = kSize * kSize;

        [[nodiscard]] static Board parse(std::string_view text);

        [[nodiscard]] std::string format() const;

        [[nodiscard]] std::optional<int> at(
            std::size_t row, std::size_t col) const;

        void set(std::size_t row, std::size_t col, int digit);

    private:
        std::array<int, kCellCount> cells{};
    };

}
