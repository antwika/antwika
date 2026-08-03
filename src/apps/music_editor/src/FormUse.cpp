#include "antwika/music_editor/FormUse.hpp"

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include <antwika/pattern/Cycle.hpp>

#include "antwika/music_editor/ScoreError.hpp"

namespace antwika::music_editor
{

    namespace
    {
        // The bound notation puts on every number it reads.
        // A form is walked per queried window rather than expanded.
        // So nothing here can amplify the way "0!2000000000" would.
        // The limit is uniformity rather than protection.
        inline constexpr std::int64_t kMaxBars = 1024;

        [[nodiscard]] std::vector<std::string_view> tokensOf(
            const std::string_view text)
        {
            std::vector<std::string_view> tokens;

            std::size_t at = 0;

            while (at < text.size())
            {
                if (text[at] == ' ' || text[at] == '\t')
                {
                    ++at;

                    continue;
                }

                const auto from = at;

                while (at < text.size() && text[at] != ' '
                       && text[at] != '\t')
                {
                    ++at;
                }

                tokens.push_back(text.substr(from, at - from));
            }

            return tokens;

            // The excluded line is the vector's unwind destructor.
            // Nothing between its construction and the return throws.
        } // GCOVR_EXCL_LINE

        [[nodiscard]] bool isNameCharacter(const char letter) noexcept
        {
            return (letter >= 'a' && letter <= 'z')
                || (letter >= 'A' && letter <= 'Z')
                || (letter >= '0' && letter <= '9') || letter == '_';
        }

        // The whole token names the offender in the refusal.
        // "verse!" and "/4" both read wrong at the same place.
        [[nodiscard]] std::string nameIn(
            const std::string_view token, const std::string_view name)
        {
            if (name.empty()
                || !std::ranges::all_of(name, isNameCharacter))
            {
                throw ScoreError(
                    std::string(token)
                    + " is not a section name: letters, digits and _");
            }

            return std::string(name);
        }

        [[nodiscard]] std::int64_t barsIn(
            const std::string_view label, const std::string_view digits)
        {
            std::int64_t value = 0;

            const auto *const last = digits.data() + digits.size();

            const auto read =
                std::from_chars(digits.data(), last, value);

            if (read.ec != std::errc{} || read.ptr != last)
            {
                throw ScoreError(
                    std::string(label)
                    + " wants a whole number of bars");
            }

            if (value < 1)
            {
                throw ScoreError(
                    std::string(label) + " plays no bars");
            }

            if (value > kMaxBars)
            {
                throw ScoreError(
                    std::string(label) + " is past the limit of "
                    + std::to_string(kMaxBars) + " bars");
            }

            return value;
        }

        [[nodiscard]] FormUse useIn(const std::string_view token)
        {
            const auto cut = token.find('/');

            if (cut == std::string_view::npos)
            {
                // The excluded line carries the aggregate's unwind pad.
                // Only a throw after nameIn returned would take it.
                // Nothing runs between its return and the aggregate's.
                // See docs/confirming-unreachable-branches.md, (a).
                return FormUse{ // GCOVR_EXCL_LINE
                    .name = nameIn(token, token)};
            }

            // The excluded line carries the aggregate's unwind pads.
            // The reachable unwind is barsIn refusing its number.
            // That one lands on barsIn's own line and is exercised.
            // What is left destroys a FormUse no throw leaves half-made.
            // See docs/confirming-unreachable-branches.md, (a).
            return FormUse{ // GCOVR_EXCL_LINE
                .name = nameIn(token, token.substr(0, cut)),
                .bars = barsIn(token, token.substr(cut + 1))};
        }
    } // namespace

    std::vector<FormUse> readFormLine(const std::string_view text)
    {
        std::vector<FormUse> uses;

        for (const auto token : tokensOf(text))
        {
            uses.push_back(useIn(token));
        }

        if (uses.empty())
        {
            throw ScoreError(
                "form: names the sections it plays, in order");
        }

        return uses;
    }

    std::int64_t readBarsLine(const std::string_view text)
    {
        const auto tokens = tokensOf(text);

        if (tokens.size() != 1)
        {
            throw ScoreError("bars: wants one whole number of cycles");
        }

        return barsIn("bars:", tokens.front());
    }

    std::vector<std::string> readPartLine(const std::string_view text)
    {
        std::vector<std::string> names;

        for (const auto token : tokensOf(text))
        {
            auto name = nameIn(token, token);

            if (std::ranges::find(names, name) == names.end())
            {
                names.push_back(std::move(name));
            }
        }

        if (names.empty())
        {
            throw ScoreError(
                "part: names the sections its voices play in");
        }

        return names;
    }

    void resolveBars(
        std::vector<FormUse> &uses, const std::int64_t fallback)
    {
        for (auto &use : uses)
        {
            if (use.bars != 0)
            {
                continue;
            }

            if (fallback == 0)
            {
                throw ScoreError(
                    use.name + " has no length: give a bars: line or "
                    + use.name + "/<n>");
            }

            use.bars = fallback;
        }
    }

    std::int64_t periodOf(const std::vector<FormUse> &uses) noexcept
    {
        std::int64_t period = 0;

        for (const auto &use : uses)
        {
            period += use.bars;
        }

        return period;
    }

    std::vector<pattern::Span> windowsFor(
        const std::vector<std::string> &parts,
        const std::vector<FormUse> &uses)
    {
        std::vector<pattern::Span> windows;

        std::int64_t reached = 0;

        for (const auto &use : uses)
        {
            const auto begin = reached;

            reached += use.bars;

            if (std::ranges::find(parts, use.name) == parts.end())
            {
                continue;
            }

            windows.emplace_back(
                pattern::Cycle(begin), pattern::Cycle(reached));
        }

        return windows;

        // The excluded line is the vector's unwind destructor.
        // Nothing between its construction and the return throws.
    } // GCOVR_EXCL_LINE

} // namespace antwika::music_editor
