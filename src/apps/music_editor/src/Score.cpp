#include "antwika/music_editor/Score.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

#include <antwika/notation/NotationError.hpp>
#include <antwika/notation/ParsePattern.hpp>
#include <antwika/pattern/PatternError.hpp>
#include <antwika/pattern/Patterns.hpp>

namespace antwika::music_editor
{

    namespace
    {
        // What a voice line opens with.
        // Two characters rather than one, so no notation begins with it.
        constexpr std::string_view kVoiceMark{"$:"};

        // What the rest of a line is, once one of these opens it.
        constexpr std::string_view kCommentMark{"//"};

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

        // The first word of a line, and what is left after it.
        [[nodiscard]] std::string_view firstWord(
            std::string_view text) noexcept
        {
            return text.substr(0, text.find_first_of(kBlanks));
        }

        [[nodiscard]] std::string_view afterFirstWord(
            std::string_view text) noexcept
        {
            const auto space = text.find_first_of(kBlanks);

            if (space == std::string_view::npos)
            {
                return {};
            }

            return trimmed(text.substr(space));
        }
    } // namespace

    std::string openingSource()
    {
        // A score that already makes something.
        // Written in the syntax the editor is for.
        return "// type at me: every keystroke lands in the music\n"
               "$: bass 0 ~ 0 [~ 3]\n"
               "$: lead <12 7> ~ 10 ~\n"
               "$: bell ~ 19 ~ [24 19]\n"
               "$: drum 0(3,8)\n";
    }

    Score::Score() : words(kNote)
    {
        tracks.reserve(kTrackCount);

        // The excluded line carries two (throw) edges.
        // It also carries an unwind pad for the strings and vector.
        // Each is taken only if an allocation actually fails.
        // See docs/confirming-unreachable-branches.md, signature (a).
        for (std::size_t track = 0; track < kTrackCount; ++track)
        {
            tracks.push_back(
                Track{ // GCOVR_EXCL_LINE
                    .source = std::string{},
                    .failure = std::string{},
                    .playing = pattern::silence()});
        }
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
        claimed.fill(false);

        std::size_t number = 0;
        std::size_t begin = 0;

        // The break below is the only way out.
        // A line always begins at or before its document's end.
        // So a condition here would be a direction nothing can take.
        while (true)
        {
            const auto end = document.find('\n', begin);

            const auto stop =
                end == std::string::npos ? document.size() : end;

            ++number;

            readLine(
                std::string_view{document}.substr(begin, stop - begin),
                number);

            if (end == std::string::npos)
            {
                break;
            }

            begin = end + 1;
        }

        // A voice no line names is a voice that was taken out.
        // Its text is forgotten too, so writing it again is heard.
        for (std::size_t track = 0; track < kTrackCount; ++track)
        {
            if (claimed[track])
            {
                continue;
            }

            tracks[track].playing = pattern::silence();
            tracks[track].source.clear();
            tracks[track].failure.clear();
        }
    }

    void Score::readLine(
        const std::string_view line, const std::size_t number)
    {
        const auto text = trimmed(line);

        if (text.empty() || text.starts_with(kCommentMark))
        {
            return;
        }

        if (!text.starts_with(kVoiceMark))
        {
            refuse(number, "a voice line opens with $:");

            return;
        }

        const auto rest = trimmed(text.substr(kVoiceMark.size()));
        const auto name = firstWord(rest);
        const auto track = trackFor(name);

        if (!track.has_value())
        {
            refuse(
                number, "name a voice: bass, lead, bell or drum");

            return;
        }

        if (claimed[*track])
        {
            refuse(number, std::string(name) + " is already sounding");

            return;
        }

        claimed[*track] = true;

        play(*track, afterFirstWord(rest), number);
    }

    void Score::play(
        const std::size_t track,
        const std::string_view notation,
        const std::size_t number)
    {
        auto &held = tracks[track];

        // Read once, however many ticks the line then sits there for.
        // A refusal is repeated, since the line is still refused.
        if (held.source == notation)
        {
            if (!held.failure.empty())
            {
                refuse(number, held.failure);
            }

            return;
        }

        held.source = notation;
        ++parsed;

        if (notation.empty())
        {
            held.playing = pattern::silence();
            held.failure.clear();

            return;
        }

        try
        {
            held.playing =
                notation::parsePattern(held.source, words);

            held.failure.clear();
        }
        // The excluded line has a third direction.
        // It is an exception neither clause below catches.
        // parsePattern raises only those two types.
        // So reaching it needs an allocation to fail inside one.
        catch (const notation::NotationError &refused) // GCOVR_EXCL_LINE
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

    const Pattern &Score::playing(const std::size_t track) const
    {
        return tracks[track].playing;
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
