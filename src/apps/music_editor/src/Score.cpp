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

        // The arrangement headers.
        // Each is a whole word together with its colon.
        // So "formless:" opens no header.
        constexpr std::string_view kFormMark{"form:"};
        constexpr std::string_view kBarsMark{"bars:"};
        constexpr std::string_view kPartMark{"part:"};

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

        using detail::trimmed;
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
                begin,
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

        // A deleted header takes what it held with it.
        // Holding on would be resilience against an intended cut.
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

        // In line order however late a pass found each problem.
        // The form's cross-checks land after every line has read.
        std::ranges::stable_sort(refusals, {}, &Problem::line);
    }

    void Score::assemble()
    {
        // The form, resolved against the bars: default.
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
            // The excluded line is the handler's no-match edge.
            // Only a non-ScoreError exception would take it.
            // resolveBars can raise nothing else.
            catch (const ScoreError &refused) // GCOVR_EXCL_LINE
            {
                refuse(formLine, refused.what());
            }
        }

        if (scheduled)
        {
            // An empty part: block is a silent section on purpose.
            // A name no header holds anywhere is the typo this catches.
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

        for (std::size_t at = 0; at < lines.size(); ++at)
        {
            // A line refused before it ever read has nothing to sound.
            if (!lines[at].sounding)
            {
                continue;
            }

            const auto &parts = lines[at].parts;

            // A voice above every header plays the whole run.
            if (!parts.has_value())
            {
                sounding.push_back(lines[at].voice);
                soundingLines.push_back(at);

                continue;
            }

            // A block's voices are silent until a form schedules them.
            if (!scheduled)
            {
                continue;
            }

            auto windows = windowsFor(*parts, uses);

            // A part the form never plays is how a section is soloed.
            if (windows.empty())
            {
                continue;
            }

            // Cannot throw.
            // The windows walk the form in order.
            // And every resolved length is at least one bar.
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

            // Where this stretch of the chain sits in the document.
            // The view is a piece of the line, so this is arithmetic.
            gathering.segments.push_back(Segment{
                .chainBegin = gathering.chain.size(),
                .documentBegin = lineBegin
                    + static_cast<std::size_t>(
                                     text.data() - line.data()),
                .length = text.size()});

            gathering.chain += text;

            return;
        }

        // Whatever this line is, the voice above it is finished.
        // So it is played before this one is refused.
        // Which keeps the problems in the order their lines are in.
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

        // A bare $: gathers no characters and so points at nothing.
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
        // The excluded line is the handler's no-match edge.
        // Only a non-ScoreError exception would take it.
        // readFormLine can raise nothing else.
        catch (const ScoreError &refused) // GCOVR_EXCL_LINE
        {
            // The last form that read keeps arranging.
            // Retyping one is half a bracket down, as a chain gets.
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
        // The excluded line is the handler's no-match edge.
        // Only a non-ScoreError exception would take it.
        // readBarsLine can raise nothing else.
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
        // The excluded line is the handler's no-match edge.
        // Only a non-ScoreError exception would take it.
        // readPartLine can raise nothing else.
        catch (const ScoreError &refused) // GCOVR_EXCL_LINE
        {
            // A header that will not read schedules nothing.
            // Merged up, its voices would sound where nobody put them.
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

        // Where the chain sits is refreshed on every read.
        // An unchanged line still moves when lines above it do.
        held.segments = gathering.segments;

        // So is which block holds it.
        // A part: header written above an unchanged line moves it.
        held.parts = gathering.parts;

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
            held.notationAt = read.notationAt;
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

    std::optional<DocumentSpan> Score::spanIn(
        const std::size_t voice,
        const std::size_t begin,
        const std::size_t length) const noexcept
    {
        // A voice that is gone has nothing left to light.
        if (voice >= soundingLines.size())
        {
            return std::nullopt;
        }

        const auto &line = lines[soundingLines[voice]];

        const auto target = line.notationAt + begin;

        for (const auto &segment : line.segments)
        {
            // Segments tile the chain from zero with no gaps.
            // So only the far edge can put a target outside one.
            if (target >= segment.chainBegin + segment.length)
            {
                continue;
            }

            const auto inside = target - segment.chainBegin;

            // Clamped to its own stretch of the chain.
            // A word cut by a gathered line's edge lights what it can.
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
