#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace antwika::sudoku
{

    /**
     * @brief A 9x9 Sudoku board of givens and blanks.
     *
     * Cells hold 0 (blank) or a digit 1-9. Row/column indices are
     * 0-based; row-major flat storage.
     */
    class Board
    {
    public:
        static constexpr std::size_t kSize = 9;
        static constexpr std::size_t kCellCount = kSize * kSize;

        /**
         * @brief Parse a flat Sudoku board string.
         * @param text Exactly 81 non-whitespace characters once
         * whitespace/newlines are stripped: each a digit 1-9, or a
         * blank marker (. or 0).
         * @return The parsed board.
         * @throws BoardFormatError if text has the wrong length after
         * stripping whitespace, or contains an invalid character.
         */
        [[nodiscard]] static Board parse(std::string_view text);

        /**
         * @brief Format this board back to its flat 81-character form.
         * @return A row-major string; blanks rendered as '.'.
         */
        [[nodiscard]] std::string format() const;

        /**
         * @brief Read one cell.
         * @param row Row index, 0-8.
         * @param col Column index, 0-8.
         * @return The digit 1-9 at (row, col), or std::nullopt if
         * blank.
         * @throws BoardFormatError if row or col is out of [0, 8].
         */
        [[nodiscard]] std::optional<int> at(
            std::size_t row, std::size_t col) const;

        /**
         * @brief Write one cell.
         * @param row Row index, 0-8.
         * @param col Column index, 0-8.
         * @param digit 0 for blank, or a digit 1-9.
         * @throws BoardFormatError if row or col is out of [0, 8], or
         * digit is outside [0, 9].
         */
        void set(std::size_t row, std::size_t col, int digit);

    private:
        std::array<int, kCellCount> cells{};
    };

} // namespace antwika::sudoku
