#pragma once

#include <cstddef>
#include <string_view>

#include <antwika/pattern/Controls.hpp>
#include <antwika/pattern/ParamId.hpp>

#include "antwika/notation/IWordReader.hpp"

namespace antwika::notation
{

    using antwika::pattern::ParamId;

    /**
     * @brief Reads every word as a whole number under one name.
     *
     * The reader nearly every caller starts with: `"0 3 5"` becomes
     * three events carrying 0, 3 and 5 under whichever ParamId was
     * given.
     *
     * A word may be written as a fraction -- `3%2` -- so a control that
     * is not a whole number is still expressible without leaving the
     * exact arithmetic.
     */
    class NumberWords final : public IWordReader
    {
    public:
        /**
         * @brief Construct a reader naming what it reads.
         * @param id What every word it reads is named.
         */
        explicit NumberWords(ParamId id) noexcept;

        /**
         * @brief Read one word as a number.
         * @param word The word.
         * @param at Where it starts; this reader has no use for it.
         * @return One control, under this reader's id.
         * @throws NotationError If the word is not a whole number or a
         * fraction of two.
         * @throws antwika::pattern::PatternError If the fraction's
         * denominator is zero, its sign cannot be moved to the
         * numerator, or the value will not fit -- "3%0" parses as a
         * word and is refused by ParamValue's own constructor.
         */
        [[nodiscard]] Controls read(
            std::string_view word, std::size_t at) const override;

    private:
        ParamId named;
    };

} // namespace antwika::notation
