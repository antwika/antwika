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

        // Every number this grammar reads is bounded.
        // A term repeated two billion times is that many patterns.
        // So is a Euclidean rhythm two billion steps long.
        // The parse dies of std::bad_alloc rather than throwing.
        // The live editor's resilience cannot survive that.
        // A line that will not parse is meant to leave one playing.
        // A thousand slots is finer than a cycle can articulate.
        // So the limit refuses nothing musical.
        inline constexpr std::int64_t kMaxNumber = 1024;

        // A speed factor gets its own limit because '*' composes.
        // Four factors of sixty-four nest into sixteen million.
        // That is a dozen characters for a window that many cycles wide.
        // Span::spanCycles walks such a window one element at a time.
        // So the product along a nesting path is what is bounded.
        inline constexpr std::int64_t kMaxSpeed = 1024;

        // '%' is here for the fraction word NumberWords reads.
        // It cannot be taken for the '%' of a ratio.
        // A ratio is read digit by digit and never asks for a word.
        [[nodiscard]] bool isWordCharacter(char letter) noexcept
        {
            return (letter >= 'a' && letter <= 'z')
                || (letter >= 'A' && letter <= 'Z')
                || (letter >= '0' && letter <= '9') || letter == '_'
                || letter == '.' || letter == '#' || letter == '+'
                || letter == '-' || letter == '%';
        }

        // A recursive descent over the string.
        // It holds where it got to and how many degradations it saw.
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

            // A tie is an underscore standing alone.
            // '_' is a word character, so "a_b" is still one word.
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

            // A stack is one or more sequences separated by commas.
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

            // A sequence is one or more terms sharing a cycle.
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

                    // A tie holds the slot before it one slot longer.
                    // Tidal writes it the same way: "0 _ 3".
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

                // A sequence of n slots plays each n times as fast.
                // A tie's slot counts: it thins everything around it.
                // So n multiplies every slot's nesting-path product.
                // Siblings alone still meet as a maximum above.
                // "0*64 3*64" is two paths of 128, never one of 4096.
                // Nested "[0!64]!64" brackets multiply, and refuse.
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

                // Every weight one is a fastcat saying the same thing.
                // A tie anywhere makes it the timecat only it can say.
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

            // A term is one factor and every modifier after it.
            // It gives back more than one slot only for '!'.
            [[nodiscard]] std::vector<Pattern> parseTerm()
            {
                auto result = parseFactor();

                // Starts at whatever the factor already carried.
                // A bracket's own factors count towards this one's.
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

                        // One count per term.
                        // Refused, never overwritten in silence.
                        // "0!2!3" used to drop its 2.
                        if (copies != 1)
                        {
                            throw NotationError(
                                "antwika::notation: a term takes one "
                                "'!' count; \"0!2!3\" says two");
                        }

                        // The count sits against its '!': "0!3 5".
                        // Tidal reads a spaced "0! 3" as 0 0 3.
                        // Eating the 3 as a count plays wrong notes.
                        // So a bare '!' is refused rather than misread.
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

                        // Counted left to right.
                        // Two in one string thin out differently.
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

                        // A Euclidean rhythm is a fastcat, steps wide.
                        // Its steps multiply density like '*' does.
                        // So they fold into the same bound.
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

                // A word and a rest each carry no speed of their own.
                // A bracket overwrites this from what is inside it.
                deepest = 1;

                // Never called at the end of the string.
                // Both callers skip space and stop before they get here.
                // A guard for it would be a branch nothing could reach.
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

                // Where the word starts is part of what it means.
                // A reader may point its controls back at the source.
                const auto wordAt = at;

                return pattern::pure(
                    words.read(parseWord(), wordAt));
            }

            // Angle brackets take one of their parts per cycle.
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

                    // A turn is a cycle, and a tie cannot stretch one.
                    // A note held across cycles is written "0/2".
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

                // Only the out-of-range answer is reachable here.
                // Every character from 'from' to 'at' is a digit.
                // So there is nothing from_chars can call malformed.
                if (read.ec != std::errc{} || value > kMaxNumber)
                {
                    throw NotationError(
                        "antwika::notation: the number at position "
                        + std::to_string(from) + " is above the limit "
                        "of " + std::to_string(kMaxNumber));
                }

                return value;
            }

            // Folds one more speed factor into the running product.
            // Both sides are bounded already.
            // So the product cannot overflow on its way to a refusal.
            [[nodiscard]] std::int64_t times(
                std::int64_t running, std::int64_t factor)
            {
                // A factor of nothing is refused by the algebra next.
                // It must not shrink the bound in the meantime.
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

            // A whole number, or two of them written with a '%'.
            // A decimal is deliberately not a speed here.
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

            // The largest speed-factor product on any path just read.
            // An out-parameter, rather than a pair in five returns.
            std::int64_t deepest = 1;
        };
    } // namespace

    Pattern parsePattern(
        std::string_view source, const IWordReader &words)
    {
        Reader reader(source, words);

        return reader.parse();
    }

} // namespace antwika::notation
