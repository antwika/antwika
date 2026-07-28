#include "antwika/sudoku/Puzzle.hpp"

namespace antwika::sudoku
{

    using antwika::wfc::AllDifferentConstraint;
    using antwika::wfc::Domain;

    // The closing braces below are gcov/NRVO misses, not real gaps.
    // See the identical case in antwika::wfc::Domain::singleton.
    // It is documented in docs/confirming-unreachable-branches.md.
    std::vector<Domain> buildInitialWave(const Board &board)
    {
        std::vector<Domain> wave;
        wave.reserve(Board::kCellCount);
        for (std::size_t row = 0; row < Board::kSize; ++row)
        {
            for (std::size_t col = 0; col < Board::kSize; ++col)
            {
                const std::optional<int> given = board.at(row, col);
                if (given.has_value())
                {
                    wave.push_back(Domain::singleton(
                        static_cast<std::size_t>(*given - 1),
                        Board::kSize));
                }
                else
                {
                    wave.emplace_back(Board::kSize);
                }
            }
        }
        return wave;
    } // GCOVR_EXCL_LINE

    std::vector<AllDifferentConstraint> buildConstraints()
    {
        std::vector<AllDifferentConstraint> constraints;
        constraints.reserve(Board::kSize * 3);

        for (std::size_t r = 0; r < Board::kSize; ++r)
        {
            std::vector<std::size_t> rowCells;
            rowCells.reserve(Board::kSize);
            for (std::size_t c = 0; c < Board::kSize; ++c)
            {
                rowCells.push_back(r * Board::kSize + c);
            }
            constraints.emplace_back(std::move(rowCells));
        }

        for (std::size_t c = 0; c < Board::kSize; ++c)
        {
            std::vector<std::size_t> colCells;
            colCells.reserve(Board::kSize);
            for (std::size_t r = 0; r < Board::kSize; ++r)
            {
                colCells.push_back(r * Board::kSize + c);
            }
            constraints.emplace_back(std::move(colCells));
        }

        for (std::size_t b = 0; b < Board::kSize; ++b)
        {
            std::vector<std::size_t> boxCells;
            boxCells.reserve(Board::kSize);
            for (std::size_t dr = 0; dr < 3; ++dr)
            {
                for (std::size_t dc = 0; dc < 3; ++dc)
                {
                    const std::size_t row = 3 * (b / 3) + dr;
                    const std::size_t col = 3 * (b % 3) + dc;
                    boxCells.push_back(row * Board::kSize + col);
                }
            }
            constraints.emplace_back(std::move(boxCells));
        }

        return constraints;
    } // GCOVR_EXCL_LINE

} // namespace antwika::sudoku
