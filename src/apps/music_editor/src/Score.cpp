#include "antwika/music_editor/Score.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include <antwika/notation/NotationError.hpp>
#include <antwika/notation/ParsePattern.hpp>
#include <antwika/pattern/Combinators.hpp>
#include <antwika/pattern/PatternError.hpp>

#include "antwika/music_editor/FormUse.hpp"
#include "antwika/music_editor/ScoreError.hpp"
#include "antwika/music_editor/VoiceChain.hpp"

#include "ScoreText.hpp"

namespace antwika::music_editor
{

    namespace
    {
        constexpr std::string_view kVoiceMark{"$:"};

        constexpr std::string_view kCommentMark{"//"};

        constexpr std::string_view kJoinMark{"."};

        constexpr std::string_view kFormMark{"form:"};
        constexpr std::string_view kBarsMark{"bars:"};
        constexpr std::string_view kPartMark{"part:"};

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

        using detail::trimmed;
    }

    std::string openingSource()
    {
        return "$: drum.n(\"0*4\").gain(.1)\n"
               "$: bass.n(\"<0 10 8 5>\").hold(400).o(-1)\n"
               "$: bell.n(\"<12 10 8 5> 7 <5 5*2> 3\")"
               ".o(-2).att(20).hold(20)\n"
               "$: bass.n(\"<0 10 8 5> [<3 5 8 7> 7]!3\")"
               ".gain(.15).pianoroll()\n";
    }

    Score::Score() = default;

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

        formPresent = false;
        barsPresent = false;
        activeParts.reset();
        partNames.clear();
        firstPartLine = 0;

        std::size_t number = 0;
        std::size_t begin = 0;

        Gathered gathering;

        while (true)
        {
            const auto end = document.find('\n', begin);

            const auto stop =
                end == std::string::npos ? document.size() : end;

            ++number;

            readLine(
                std::string_view{document}.substr(begin, stop - begin),
                number,
                begin,
                gathering);

            if (end == std::string::npos)
            {
                break;
            }

            begin = end + 1;
        }

        finish(gathering);

        lines.resize(seen);

        if (!formPresent)
        {
            formEver = false;
            formHeld.clear();
        }

        if (!barsPresent)
        {
            barsEver = false;
        }

        assemble();

        std::ranges::stable_sort(refusals, {}, &Problem::line);
    }

    void Score::assemble()
    {
        std::vector<FormUse> uses;
        std::int64_t period = 0;
        bool scheduled = false;

        if (formEver)
        {
            try
            {
                uses = formHeld;
                resolveBars(uses, barsEver ? barsHeld : 0);
                period = periodOf(uses);
                scheduled = true;
            }
            catch (const ScoreError &refused) // GCOVR_EXCL_LINE
            {
                refuse(formLine, refused.what());
            }
        }

        if (scheduled)
        {
            for (const auto &use : uses)
            {
                if (std::ranges::find(partNames, use.name)
                    != partNames.end())
                {
                    continue;
                }

                const Problem missing{
                    .line = formLine,
                    .message = "no part: holds " + use.name};

                if (std::ranges::find(refusals, missing)
                    == refusals.end())
                {
                    refusals.push_back(missing);
                }
            }
        }

        if (!formPresent && firstPartLine != 0)
        {
            refuse(firstPartLine, "no form: says when these parts play");
        }

        sounding.clear();
        soundingLines.clear();
        rolls.clear();
        waves.clear();

        for (std::size_t at = 0; at < lines.size(); ++at)
        {
            const auto &held = lines[at];

            if ((!held.pianoroll && !held.waveform)
                || held.segments.empty())
            {
                continue;
            }

            const std::string_view before{
                document.data(), held.segments.back().documentBegin};

            const auto newlines = std::ranges::count(before, '\n');
            const auto below = static_cast<std::size_t>(newlines);

            if (held.pianoroll)
            {
                rolls.push_back(Pianoroll{ // GCOVR_EXCL_LINE
                    .playing = held.voice.playing,
                    .preset = held.voice.preset,
                    .line = below});
            }

            if (held.waveform)
            {
                waves.push_back(Waveform{ // GCOVR_EXCL_LINE
                    .playing = held.voice.playing,
                    .preset = held.voice.preset,
                    .line = below,
                    .chain = held.chain});
            }
        }

        for (std::size_t at = 0; at < lines.size(); ++at)
        {
            if (!lines[at].sounding)
            {
                continue;
            }

            const auto &parts = lines[at].parts;

            if (!parts.has_value())
            {
                sounding.push_back(lines[at].voice);
                soundingLines.push_back(at);

                continue;
            }

            if (!scheduled)
            {
                continue;
            }

            auto windows = windowsFor(*parts, uses);

            if (windows.empty())
            {
                continue;
            }

            sounding.push_back(
                Voice{
                    .preset = lines[at].voice.preset,
                    .playing = pattern::during(
                        period,
                        std::move(windows),
                        lines[at].voice.playing)});

            soundingLines.push_back(at);
        }
    }

    void Score::readLine(
        const std::string_view line,
        const std::size_t number,
        const std::size_t lineBegin,
        Gathered &gathering)
    {
        const auto text = trimmed(uncommented(line));

        if (text.empty())
        {
            return;
        }

        if (text.starts_with(kJoinMark))
        {
            if (gathering.opened == 0)
            {
                refuse(number, "a call above no voice line");

                return;
            }

            gathering.segments.push_back(Segment{
                .chainBegin = gathering.chain.size(),
                .documentBegin = lineBegin
                    + static_cast<std::size_t>(
                                     text.data() - line.data()),
                .length = text.size()});

            gathering.chain += text;

            return;
        }

        finish(gathering);

        if (text.starts_with(kFormMark))
        {
            readForm(
                trimmed(text.substr(kFormMark.size())), number);

            return;
        }

        if (text.starts_with(kBarsMark))
        {
            readBars(
                trimmed(text.substr(kBarsMark.size())), number);

            return;
        }

        if (text.starts_with(kPartMark))
        {
            readPart(
                trimmed(text.substr(kPartMark.size())), number);

            return;
        }

        if (!text.starts_with(kVoiceMark))
        {
            refuse(
                number, "a line opens with $:, form:, bars: or part:");

            return;
        }

        const auto content = trimmed(text.substr(kVoiceMark.size()));

        gathering.chain = std::string(content);
        gathering.segments.clear();
        gathering.parts = activeParts;
        gathering.opened = number;

        if (!content.empty())
        {
            gathering.segments.push_back(Segment{
                .chainBegin = 0,
                .documentBegin = lineBegin
                    + static_cast<std::size_t>(
                                     content.data() - line.data()),
                .length = content.size()});
        }
    }

    void Score::readForm(
        const std::string_view text, const std::size_t number)
    {
        if (formPresent)
        {
            refuse(number, "one form: per score");

            return;
        }

        formPresent = true;
        formLine = number;

        try
        {
            formHeld = readFormLine(text);
            formEver = true;
        }
        catch (const ScoreError &refused) // GCOVR_EXCL_LINE
        {
            refuse(number, refused.what());
        }
    }

    void Score::readBars(
        const std::string_view text, const std::size_t number)
    {
        if (barsPresent)
        {
            refuse(number, "one bars: per score");

            return;
        }

        barsPresent = true;

        try
        {
            barsHeld = readBarsLine(text);
            barsEver = true;
        }
        catch (const ScoreError &refused) // GCOVR_EXCL_LINE
        {
            refuse(number, refused.what());
        }
    }

    void Score::readPart(
        const std::string_view text, const std::size_t number)
    {
        if (firstPartLine == 0)
        {
            firstPartLine = number;
        }

        try
        {
            activeParts = readPartLine(text);

            for (const auto &name : *activeParts)
            {
                if (std::ranges::find(partNames, name)
                    == partNames.end())
                {
                    partNames.push_back(name);
                }
            }
        }
        catch (const ScoreError &refused) // GCOVR_EXCL_LINE
        {
            activeParts.emplace();

            refuse(number, refused.what());
        }
    }

    void Score::finish(Gathered &gathering)
    {
        if (gathering.opened == 0)
        {
            return;
        }

        play(gathering);

        gathering.chain.clear();
        gathering.segments.clear();
        gathering.opened = 0;
    }

    void Score::play(const Gathered &gathering)
    {
        const std::string &chain = gathering.chain;
        const std::size_t number = gathering.opened;

        const auto at = seen++;

        if (at == lines.size())
        {
            lines.emplace_back();
        }

        auto &held = lines[at];

        held.segments = gathering.segments;

        held.parts = gathering.parts;

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

            Voice voice{
                .preset = read.preset,
                .playing =
                    notation::parsePattern(read.notation, words)};

            held.voice = std::move(voice);
            held.notationAt = read.notationAt;
            held.pianoroll = read.pianoroll;
            held.waveform = read.waveform;
            held.sounding = true;
            held.failure.clear();
        }
        catch (const ScoreError &refused) // GCOVR_EXCL_LINE
        {
            held.failure = refused.what();
        }
        catch (const notation::NotationError &refused)
        {
            held.failure = refused.what();
        }
        catch (const pattern::PatternError &refused)
        {
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

    std::optional<DocumentSpan> Score::spanIn(
        const std::size_t voice,
        const std::size_t begin,
        const std::size_t length) const noexcept
    {
        if (voice >= soundingLines.size())
        {
            return std::nullopt;
        }

        const auto &line = lines[soundingLines[voice]];

        const auto target = line.notationAt + begin;

        for (const auto &segment : line.segments)
        {
            if (target >= segment.chainBegin + segment.length)
            {
                continue;
            }

            const auto inside = target - segment.chainBegin;

            const auto take =
                std::min(length, segment.length - inside);

            if (take == 0)
            {
                return std::nullopt;
            }

            const auto docBegin = segment.documentBegin + inside;

            return DocumentSpan{
                .begin = docBegin, .end = docBegin + take};
        }

        return std::nullopt;
    }

    std::string_view Score::chainOf(
        const std::size_t voice) const noexcept
    {
        if (voice >= soundingLines.size())
        {
            return {};
        }

        return lines[soundingLines[voice]].chain;
    }

    const std::vector<Pianoroll> &Score::pianorolls() const noexcept
    {
        return rolls;
    }

    const std::vector<Waveform> &Score::waveforms() const noexcept
    {
        return waves;
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

}
