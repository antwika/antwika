#include "antwika/notation/NumberWords.hpp"

#include <charconv>
#include <cstdint>
#include <string>
#include <string_view>
#include <system_error>

#include <antwika/pattern/Controls.hpp>
#include <antwika/pattern/ParamId.hpp>
#include <antwika/pattern/ParamValue.hpp>

#include "antwika/notation/NotationError.hpp"

namespace antwika::notation
{

    namespace
    {
        [[nodiscard]] std::int64_t wholeNumber(
            std::string_view text, std::string_view word)
        {
            std::int64_t value = 0;

            const auto *const last = text.data() + text.size();
            const auto read = std::from_chars(text.data(), last, value);

            if (read.ec != std::errc{} || read.ptr != last)
            {
                throw NotationError(
                    "antwika::notation: '" + std::string(word)
                    + "' is not a number");
            }

            return value;
        }
    } // namespace

    NumberWords::NumberWords(ParamId id) noexcept : named(id)
    {
    }

    Controls NumberWords::read(std::string_view word) const
    {
        const auto divide = word.find('%');

        if (divide == std::string_view::npos)
        {
            return Controls(
                named, pattern::ParamValue(wholeNumber(word, word)));
        }

        const auto top = word.substr(0, divide);
        const auto bottom = word.substr(divide + 1);

        if (top.empty() || bottom.empty())
        {
            throw NotationError(
                "antwika::notation: '" + std::string(word)
                + "' is not a fraction");
        }

        return Controls(
            named,
            pattern::ParamValue(
                wholeNumber(top, word), wholeNumber(bottom, word)));
    }

} // namespace antwika::notation
