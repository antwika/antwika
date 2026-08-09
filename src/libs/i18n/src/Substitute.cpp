#include "antwika/i18n/Substitute.hpp"

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace antwika::i18n
{

    namespace
    {

        constexpr std::size_t kMaxIndexDigits{4};

        std::optional<std::size_t> parseIndex(std::string_view body) noexcept
        {
            if (body.empty() || body.size() > kMaxIndexDigits)
            {
                return std::nullopt;
            }

            std::size_t index{0};

            for (const char digit : body)
            {
                if (digit < '0' || digit > '9')
                {
                    return std::nullopt;
                }

                index = index * 10
                    + static_cast<std::size_t>(digit - '0');
            }

            return index;
        }

    }

    std::string substitute(
        std::string_view pattern, std::span<const std::string_view> args)
    {
        std::string result;
        result.reserve(pattern.size());

        std::size_t cursor{0};

        while (cursor < pattern.size())
        {
            const std::size_t open = pattern.find('{', cursor);

            if (open == std::string_view::npos)
            {
                result.append(pattern.substr(cursor));
                return result;
            }

            result.append(pattern.substr(cursor, open - cursor));

            const std::size_t close = pattern.find('}', open + 1);

            const std::optional<std::size_t> index =
                close == std::string_view::npos
                ? std::nullopt
                : parseIndex(pattern.substr(open + 1, close - open - 1));

            if (index.has_value() && *index < args.size())
            {
                result.append(args[*index]);
                cursor = close + 1;
            }
            else
            {
                result.push_back('{');
                cursor = open + 1;
            }
        }

        return result;
    } // GCOVR_EXCL_LINE

}
