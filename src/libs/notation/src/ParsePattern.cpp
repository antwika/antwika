#include "antwika/notation/ParsePattern.hpp"

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include <antwika/pattern/Combinators.hpp>
#include <antwika/pattern/Cycle.hpp>
#include <antwika/pattern/ParamValue.hpp>
#include <antwika/pattern/Pattern.hpp>
#include <antwika/pattern/Patterns.hpp>

#include "antwika/notation/IWordReader.hpp"
#include "antwika/notation/NotationError.hpp"

namespace antwika::notation
{

    namespace
    {
        using antwika::pattern::Cycle;
        using antwika::pattern::ParamValue;

        inline constexpr std::int64_t kMaxNumber = 1024;

        inline constexpr std::int64_t kMaxSpeed = 1024;

        [[nodiscard]] bool isWordCharacter(char letter) noexcept
        {
            return (letter >= 'a' && letter <= 'z')
                || (letter >= 'A' && letter <= 'Z')
                || (letter >= '0' && letter <= '9') || letter == '_'
                || letter == '.' || letter == '#' || letter == '+'
                || letter == '-' || letter == '%';
        }

        class Reader final
        {
        public:
            Reader(std::string_view text, const IWordReader &reader)
                : source(text), words(reader)
            {
            }

            [[nodiscard]] Pattern parse()
            {
                auto result = parseStack();

                skipSpace();

                if (!atEnd())
                {
                    throw NotationError(
                        "antwika::notation: nothing in this pattern "
                        "opened the '" + std::string(1, peek())
                        + "' at position " + std::to_string(at));
                }

                return result;
            }

        private:
            [[nodiscard]] bool atEnd() const noexcept
            {
                return at >= source.size();
            }

            [[nodiscard]] char peek() const noexcept
            {
                return source[at];
            }

            void skipSpace() noexcept
            {
                while (!atEnd() && (peek() == ' ' || peek() == '\t'))
                {
                    ++at;
                }
            }

            [[nodiscard]] bool nextIs(char letter) noexcept
            {
                skipSpace();

                return !atEnd() && peek() == letter;
            }

            [[nodiscard]] bool nextIsTie() noexcept
            {
                if (!nextIs('_'))
                {
                    return false;
                }

                return at + 1 >= source.size()
                    || !isWordCharacter(source[at + 1]);
            }

            void expect(char letter)
            {
                if (!nextIs(letter))
                {
                    throw NotationError(
                        "antwika::notation: expected '"
                        + std::string(1, letter) + "' at position "
                        + std::to_string(at));
                }

                ++at;
            }

            [[nodiscard]] Pattern parseStack()
            {
                std::vector<Pattern> layers;

                layers.push_back(parseSequence());

                auto widest = deepest;

                while (nextIs(','))
                {
                    ++at;
                    layers.push_back(parseSequence());
                    widest = std::max(widest, deepest);
                }

                deepest = widest;

                if (layers.size() == 1)
                {
                    return layers.front();
                }

                return pattern::stack(std::move(layers));
            }

            [[nodiscard]] Pattern parseSequence()
            {
                std::vector<Pattern> slots;
                std::vector<std::int64_t> weights;
                std::int64_t widest = 1;

                for (;;)
                {
                    skipSpace();

                    if (atEnd() || peek() == ',' || peek() == ']'
                        || peek() == '>')
                    {
                        break;
                    }

                    if (nextIsTie())
                    {
                        if (slots.empty())
                        {
                            throw NotationError(
                                "antwika::notation: the tie at "
                                "position " + std::to_string(at)
                                + " has no slot before it to hold");
                        }

                        ++at;
                        ++weights.back();

                        continue;
                    }

                    for (auto &slot : parseTerm())
                    {
                        slots.push_back(std::move(slot));
                        weights.push_back(1);
                    }

                    widest = std::max(widest, deepest);
                }

                if (slots.empty())
                {
                    throw NotationError(
                        "antwika::notation: a sequence at position "
                        + std::to_string(at) + " holds nothing");
                }

                std::int64_t total = 0;

                for (const auto weight : weights)
                {
                    total += weight;
                }

                deepest = times(widest, total);

                if (slots.size() == 1)
                {
                    return slots.front();
                }

                if (total == static_cast<std::int64_t>(slots.size()))
                {
                    return pattern::fastcat(std::move(slots));
                }

                std::vector<pattern::Slice> slices;
                slices.reserve(slots.size());

                for (std::size_t slot = 0; slot < slots.size(); ++slot)
                {
                    slices.push_back(
                        pattern::Slice{
                            .weight = Cycle(weights[slot]),
                            .part = std::move(slots[slot])});
                }

                return pattern::timecat(std::move(slices));
            }

            [[nodiscard]] std::vector<Pattern> parseTerm()
            {
                auto result = parseFactor();

                auto speed = deepest;

                std::int64_t copies = 1;

                for (;;)
                {
                    if (nextIs('*'))
                    {
                        ++at;
                        const auto ratio = parseRatio();
                        speed = times(speed, ratio.numerator());
                        result =
                            pattern::fast(ratio, std::move(result));
                    }
                    else if (nextIs('/'))
                    {
                        ++at;
                        const auto ratio = parseRatio();
                        speed = times(speed, ratio.denominator());
                        result =
                            pattern::slow(ratio, std::move(result));
                    }
                    else if (nextIs('!'))
                    {
                        ++at;

                        if (copies != 1)
                        {
                            throw NotationError(
                                "antwika::notation: a term takes one "
                                "'!' count; \"0!2!3\" says two");
                        }

                        if (atEnd() || peek() < '0' || peek() > '9')
                        {
                            throw NotationError(
                                "antwika::notation: '!' wants its count "
                                "against it, as in \"0!3\"; the bare "
                                "'!' Tidal repeats the previous term "
                                "with is not in this grammar");
                        }

                        copies = parseWholeNumber();
                    }
                    else if (nextIs('?'))
                    {
                        ++at;

                        result = pattern::degradeBy(
                            ParamValue(1, 2), degradations++,
                            std::move(result));
                    }
                    else if (nextIs('('))
                    {
                        ++at;
                        const auto pulses = parseWholeNumber();
                        expect(',');
                        const auto steps = parseWholeNumber();
                        expect(')');

                        speed = times(speed, steps);

                        result = pattern::euclid(
                            pulses, steps, std::move(result));
                    }
                    else
                    {
                        break;
                    }
                }

                deepest = speed;

                if (copies < 1)
                {
                    throw NotationError(
                        "antwika::notation: a term cannot occupy "
                        + std::to_string(copies) + " slots");
                }

                std::vector<Pattern> slots;
                slots.reserve(static_cast<std::size_t>(copies));

                for (std::int64_t copy = 0; copy < copies; ++copy)
                {
                    slots.push_back(result);
                }

                return slots;
            }

            [[nodiscard]] Pattern parseFactor()
            {
                skipSpace();

                deepest = 1;

                if (peek() == '~')
                {
                    ++at;

                    return pattern::silence();
                }

                if (peek() == '[')
                {
                    ++at;
                    auto inner = parseStack();
                    expect(']');

                    return inner;
                }

                if (peek() == '<')
                {
                    ++at;
                    auto inner = parseAlternation();
                    expect('>');

                    return inner;
                }

                const auto wordAt = at;

                return pattern::pure(
                    words.read(parseWord(), wordAt));
            }

            [[nodiscard]] Pattern parseAlternation()
            {
                std::vector<Pattern> parts;
                std::int64_t widest = 1;

                for (;;)
                {
                    skipSpace();

                    if (atEnd() || peek() == '>')
                    {
                        break;
                    }

                    if (nextIsTie())
                    {
                        throw NotationError(
                            "antwika::notation: the tie at position "
                            + std::to_string(at) + " cannot hold an "
                            "alternation's turn; write 0/2 to hold a "
                            "note across cycles");
                    }

                    for (auto &part : parseTerm())
                    {
                        parts.push_back(std::move(part));
                    }

                    widest = std::max(widest, deepest);
                }

                deepest = widest;

                if (parts.empty())
                {
                    throw NotationError(
                        "antwika::notation: an alternation at position "
                        + std::to_string(at) + " holds nothing");
                }

                return pattern::slowcat(std::move(parts));
            }

            [[nodiscard]] std::string_view parseWord()
            {
                const auto from = at;

                while (!atEnd() && isWordCharacter(peek()))
                {
                    ++at;
                }

                if (at == from)
                {
                    throw NotationError(
                        "antwika::notation: '" + std::string(1, peek())
                        + "' at position " + std::to_string(at)
                        + " begins nothing this grammar knows");
                }

                return source.substr(from, at - from);
            }

            [[nodiscard]] std::int64_t parseWholeNumber()
            {
                skipSpace();

                const auto from = at;

                while (!atEnd() && peek() >= '0' && peek() <= '9')
                {
                    ++at;
                }

                if (at == from)
                {
                    throw NotationError(
                        "antwika::notation: a number was expected at "
                        "position " + std::to_string(at));
                }

                const auto digits = source.substr(from, at - from);

                std::int64_t value = 0;

                const auto *const last = digits.data() + digits.size();
                const auto read =
                    std::from_chars(digits.data(), last, value);

                if (read.ec != std::errc{} || value > kMaxNumber)
                {
                    throw NotationError(
                        "antwika::notation: the number at position "
                        + std::to_string(from) + " is above the limit "
                        "of " + std::to_string(kMaxNumber));
                }

                return value;
            }

            [[nodiscard]] std::int64_t times(
                std::int64_t running, std::int64_t factor)
            {
                const auto product =
                    running * std::max<std::int64_t>(factor, 1);

                if (product > kMaxSpeed)
                {
                    throw NotationError(
                        "antwika::notation: the speed factors around "
                        "position " + std::to_string(at)
                        + " multiply to " + std::to_string(product)
                        + ", above the limit of "
                        + std::to_string(kMaxSpeed));
                }

                return product;
            }

            [[nodiscard]] Cycle parseRatio()
            {
                const auto top = parseWholeNumber();

                if (!nextIs('%'))
                {
                    return Cycle(top);
                }

                ++at;

                return Cycle(top, parseWholeNumber());
            }

            std::string_view source;
            const IWordReader &words;
            std::size_t at = 0;
            std::uint64_t degradations = 0;

            std::int64_t deepest = 1;
        };
    }

    Pattern parsePattern(
        std::string_view source, const IWordReader &words)
    {
        Reader reader(source, words);

        return reader.parse();
    }

}
