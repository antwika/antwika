#include "antwika/notation/ParsePattern.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
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

        [[nodiscard]] bool isWordCharacter(char letter) noexcept
        {
            return (letter >= 'a' && letter <= 'z')
                || (letter >= 'A' && letter <= 'Z')
                || (letter >= '0' && letter <= '9') || letter == '_'
                || letter == '.' || letter == '#' || letter == '+'
                || letter == '-';
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

                while (nextIs(','))
                {
                    ++at;
                    layers.push_back(parseSequence());
                }

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

                for (;;)
                {
                    skipSpace();

                    if (atEnd() || peek() == ',' || peek() == ']'
                        || peek() == '>')
                    {
                        break;
                    }

                    for (auto &slot : parseTerm())
                    {
                        slots.push_back(std::move(slot));
                    }
                }

                if (slots.empty())
                {
                    throw NotationError(
                        "antwika::notation: a sequence at position "
                        + std::to_string(at) + " holds nothing");
                }

                if (slots.size() == 1)
                {
                    return slots.front();
                }

                return pattern::fastcat(std::move(slots));
            }

            // A term is one factor and every modifier after it.
            // It gives back more than one slot only for '!'.
            [[nodiscard]] std::vector<Pattern> parseTerm()
            {
                auto result = parseFactor();
                std::int64_t copies = 1;

                for (;;)
                {
                    if (nextIs('*'))
                    {
                        ++at;
                        result = pattern::fast(
                            parseRatio(), std::move(result));
                    }
                    else if (nextIs('/'))
                    {
                        ++at;
                        result = pattern::slow(
                            parseRatio(), std::move(result));
                    }
                    else if (nextIs('!'))
                    {
                        ++at;
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

                        result = pattern::euclid(
                            pulses, steps, std::move(result));
                    }
                    else
                    {
                        break;
                    }
                }

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

                return pattern::pure(words.read(parseWord()));
            }

            // Angle brackets take one of their parts per cycle.
            [[nodiscard]] Pattern parseAlternation()
            {
                std::vector<Pattern> parts;

                for (;;)
                {
                    skipSpace();

                    if (atEnd() || peek() == '>')
                    {
                        break;
                    }

                    for (auto &part : parseTerm())
                    {
                        parts.push_back(std::move(part));
                    }
                }

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

                std::int64_t value = 0;

                for (auto index = from; index < at; ++index)
                {
                    value = value * 10
                        + static_cast<std::int64_t>(source[index] - '0');
                }

                return value;
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
        };
    } // namespace

    Pattern parsePattern(
        std::string_view source, const IWordReader &words)
    {
        Reader reader(source, words);

        return reader.parse();
    }

} // namespace antwika::notation
