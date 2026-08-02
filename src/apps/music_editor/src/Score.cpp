#include "antwika/music_editor/Score.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

#include <antwika/notation/NotationError.hpp>
#include <antwika/notation/ParsePattern.hpp>
#include <antwika/pattern/PatternError.hpp>

#include "antwika/music_editor/ScoreError.hpp"
#include "antwika/music_editor/VoiceChain.hpp"

namespace antwika::music_editor
{

    namespace
    {
        // What a voice line opens with.
        // Two characters rather than one, so no chain begins with it.
        constexpr std::string_view kVoiceMark{"$:"};

        // What the rest of a line is, once one of these opens it.
        constexpr std::string_view kCommentMark{"//"};

        // What a line carrying the one above it on opens with.
        // The join rather than something at the end of the line above.
        // So a chain half written still reads as calls all the way down.
        // And so no line has to be edited to add another under it.
        constexpr std::string_view kJoinMark{"."};

        // Cut a line at the comment that ends it, if one does.
        // Outside quotes alone, so a notation is never cut into.
        [[nodiscard]] std::string_view uncommented(
            const std::string_view line) noexcept
        {
            bool quoted = false;

            for (std::size_t at = 0; at + 1 < line.size(); ++at)
            {
                if (line[at] == '"')
                {
                    quoted = !quoted;
                }
                else if (
                    !quoted
                    && line.substr(at, kCommentMark.size())
                        == kCommentMark)
                {
                    return line.substr(0, at);
                }
            }

            return line;
        }

        constexpr std::string_view kBlanks{" \t"};

        [[nodiscard]] std::string_view trimmed(
            std::string_view text) noexcept
        {
            const auto first = text.find_first_not_of(kBlanks);

            if (first == std::string_view::npos)
            {
                return {};
            }

            const auto last = text.find_last_not_of(kBlanks);

            return text.substr(first, last - first + 1);
        }
    } // namespace

    std::string openingSource()
    {
        // A score that already makes something.
        // And that shows the shape of the language while it does.
        // Two drums, because one of a kind is not the rule here.
        return "// type at me: every keystroke lands in the music\n"
               "$: bass.n(\"0 ~ 0 [~ 3]\").o(-1)\n"
               "$: lead.n(\"<12 7> ~ 10 ~\").gain(.18)\n"
               "$: drum.n(\"0(3,8)\")\n"
               "$: drum.n(\"~ [0 0] ~ 0\")\n"
               "    .gain(.12).pan(.5)\n";
    }

    Score::Score() : words(kNote)
    {
    }

    void Score::read(const std::string &source)
    {
        if (everRead && document == source)
        {
            return;
        }

        document = source;
        everRead = true;

        refusals.clear();
        seen = 0;

        std::size_t number = 0;
        std::size_t begin = 0;

        // A voice may be spread over several lines.
        // So it is gathered here and played once something ends it.
        Gathered gathering;

        // The break below is the only way out of this loop.
        // A condition here would be a direction nothing takes.
        // A line begins at or before the end of its document.
        while (true)
        {
            const auto end = document.find('\n', begin);

            const auto stop =
                end == std::string::npos ? document.size() : end;

            ++number;

            readLine(
                std::string_view{document}.substr(begin, stop - begin),
                number,
                gathering);

            if (end == std::string::npos)
            {
                break;
            }

            begin = end + 1;
        }

        // The last voice has no line after it to end it.
        finish(gathering);

        // A line that is gone takes its voice with it.
        lines.resize(seen);

        sounding.clear();

        for (const auto &line : lines)
        {
            // A line refused before it ever read has nothing to sound.
            if (!line.sounding)
            {
                continue;
            }

            sounding.push_back(line.voice);
        }
    }

    void Score::readLine(
        const std::string_view line,
        const std::size_t number,
        Gathered &gathering)
    {
        // A comment is cut off wherever it starts.
        // So a line that is only a comment is a line holding nothing.
        const auto text = trimmed(uncommented(line));

        if (text.empty())
        {
            return;
        }

        // A call on a line of its own joins the chain above it.
        if (text.starts_with(kJoinMark))
        {
            if (gathering.opened == 0)
            {
                refuse(number, "a call above no voice line");

                return;
            }

            gathering.chain += text;

            return;
        }

        // Whatever this line is, the voice above it is finished.
        // So it is played before this one is refused.
        // Which keeps the problems in the order their lines are in.
        finish(gathering);

        if (!text.starts_with(kVoiceMark))
        {
            refuse(number, "a voice line opens with $:");

            return;
        }

        gathering.chain = trimmed(text.substr(kVoiceMark.size()));
        gathering.opened = number;
    }

    void Score::finish(Gathered &gathering)
    {
        if (gathering.opened == 0)
        {
            return;
        }

        play(gathering.chain, gathering.opened);

        gathering.chain.clear();
        gathering.opened = 0;
    }

    void Score::play(
        const std::string_view chain, const std::size_t number)
    {
        const auto at = seen++;

        if (at == lines.size())
        {
            lines.emplace_back();
        }

        auto &held = lines[at];

        // Read once, however many ticks the line then sits there for.
        // A refusal is repeated, since the line is still refused.
        if (held.ever && held.chain == chain)
        {
            if (!held.failure.empty())
            {
                refuse(number, held.failure);
            }

            return;
        }

        held.chain = chain;
        held.ever = true;
        ++parsed;

        try
        {
            const auto read = parseVoiceChain(chain);

            // Built whole and then handed over.
            // A refused notation leaves the voice as it was.
            Voice voice{
                .preset = read.preset,
                .playing =
                    notation::parsePattern(read.notation, words)};

            held.voice = std::move(voice);
            held.sounding = true;
            held.failure.clear();
        }
        // The excluded line is the catch chain's dispatch.
        // Its one unexercised edge is a fourth kind of exception.
        // Nothing this reads can raise one.
        catch (const ScoreError &refused) // GCOVR_EXCL_LINE
        {
            // The chain itself, rather than what it plays.
            // A control no voice has, or a preset nothing is called.
            held.failure = refused.what();
        }
        catch (const notation::NotationError &refused)
        {
            // The line keeps playing whatever it last did.
            // Half a bracket is typed on the way to a whole one.
            held.failure = refused.what();
        }
        catch (const pattern::PatternError &refused)
        {
            // It read cleanly and asked for something impossible.
            // The algebra refused it, and that reads the same here.
            held.failure = refused.what();
        }

        if (!held.failure.empty())
        {
            refuse(number, held.failure);
        }
    }

    void Score::refuse(const std::size_t number, std::string message)
    {
        refusals.push_back(
            Problem{.line = number, .message = std::move(message)});
    }

    const std::vector<Voice> &Score::voices() const noexcept
    {
        return sounding;
    }

    const std::vector<Problem> &Score::problems() const noexcept
    {
        return refusals;
    }

    bool Score::hasError() const noexcept
    {
        return !refusals.empty();
    }

    std::size_t Score::reparses() const noexcept
    {
        return parsed;
    }

} // namespace antwika::music_editor
