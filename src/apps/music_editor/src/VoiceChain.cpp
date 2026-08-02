#include "antwika/music_editor/VoiceChain.hpp"

#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include <antwika/synth/Filter.hpp>
#include <antwika/synth/Waveshape.hpp>

#include "antwika/music_editor/ScoreError.hpp"

namespace antwika::music_editor
{

    namespace
    {
        using antwika::synth::FilterMode;
        using antwika::synth::Waveshape;

        constexpr std::string_view kControls{
            "n s base o trans gain pan att dec sus rel hold lpf hpf "
            "bpf res slide"};

        constexpr std::array<std::string_view, 5> kShapes{
            "sine", "saw", "square", "triangle", "noise"};

        // Twelve semitones to the octave.
        // The only music theory the language contains.
        constexpr std::int32_t kSemitonesPerOctave = 12;

        [[nodiscard]] std::string_view trimmed(
            std::string_view text) noexcept
        {
            const auto first = text.find_first_not_of(" \t");

            if (first == std::string_view::npos)
            {
                return {};
            }

            return text.substr(
                first, text.find_last_not_of(" \t") - first + 1);
        }

        // Split at the dots that join calls, and at no others.
        // A dot inside parentheses is a decimal point or notation.
        [[nodiscard]] std::vector<std::string_view> segmentsOf(
            const std::string_view chain)
        {
            std::vector<std::string_view> segments;

            std::size_t begin = 0;
            std::size_t depth = 0;
            bool quoted = false;

            for (std::size_t at = 0; at < chain.size(); ++at)
            {
                const auto letter = chain[at];

                if (letter == '"')
                {
                    quoted = !quoted;
                }
                else if (!quoted && letter == '(')
                {
                    ++depth;
                }
                else if (!quoted && letter == ')' && depth > 0)
                {
                    --depth;
                }
                else if (!quoted && depth == 0 && letter == '.')
                {
                    segments.push_back(
                        chain.substr(begin, at - begin));

                    begin = at + 1;
                }
            }

            segments.push_back(chain.substr(begin));

            return segments;

            // The excluded line is the vector's unwind destructor.
            // Nothing between its construction and the return throws.
        } // GCOVR_EXCL_LINE

        [[nodiscard]] double numberIn(
            std::string_view name, std::string_view argument)
        {
            // A leading dot is how a gain reads best.
            // std::from_chars wants a digit before it.
            std::string text{argument};

            if (text.starts_with("."))
            {
                text.insert(0, "0");
            }
            else if (text.starts_with("-."))
            {
                text.insert(1, "0");
            }

            double value = 0.0;

            const auto *const stop = text.data() + text.size();

            const auto read =
                std::from_chars(text.data(), stop, value);

            if (read.ec != std::errc{} || read.ptr != stop)
            {
                throw ScoreError(
                    std::string(name) + "(" + std::string(argument)
                    + ") wants a number");
            }

            return value;
        }

        [[nodiscard]] std::int32_t wholeIn(
            std::string_view name, std::string_view argument)
        {
            const auto value = numberIn(name, argument);
            const auto whole = static_cast<std::int32_t>(value);

            if (static_cast<double>(whole) != value)
            {
                throw ScoreError(
                    std::string(name) + "(" + std::string(argument)
                    + ") wants a whole number");
            }

            return whole;
        }

        [[nodiscard]] std::uint32_t millisecondsIn(
            std::string_view name, std::string_view argument)
        {
            const auto whole = wholeIn(name, argument);

            if (whole < 0)
            {
                throw ScoreError(
                    std::string(name) + " cannot be negative");
            }

            return static_cast<std::uint32_t>(whole);
        }

        [[nodiscard]] float withinOne(
            std::string_view name, std::string_view argument)
        {
            const auto value = numberIn(name, argument);

            if (value < -1.0 || value > 1.0)
            {
                throw ScoreError(
                    std::string(name) + " lies between -1 and 1");
            }

            return static_cast<float>(value);
        }

        [[nodiscard]] double positiveIn(
            std::string_view name, std::string_view argument)
        {
            const auto value = numberIn(name, argument);

            if (value <= 0.0)
            {
                throw ScoreError(
                    std::string(name) + " wants more than nothing");
            }

            return value;
        }

        [[nodiscard]] Waveshape shapeIn(std::string_view argument)
        {
            for (std::size_t at = 0; at < kShapes.size(); ++at)
            {
                if (kShapes[at] == argument)
                {
                    return static_cast<Waveshape>(at);
                }
            }

            throw ScoreError(
                "s(" + std::string(argument)
                + ") names no shape: sine saw square triangle noise");
        }

        [[nodiscard]] std::string quotedIn(std::string_view argument)
        {
            if (argument.size() < 2 || !argument.starts_with('"')
                || !argument.ends_with('"'))
            {
                throw ScoreError("n(...) wants its notation in quotes");
            }

            return std::string(argument.substr(1, argument.size() - 2));
        }

        void applyCall(
            VoiceChain &voice,
            const std::string_view name,
            const std::string_view argument,
            const std::string_view whole)
        {
            auto &preset = voice.preset;

            if (name == "n")
            {
                voice.notation = quotedIn(argument);

                // Where the notation's characters begin in the chain.
                // One past the argument's opening quote.
                // The argument is a view into the chain.
                // So this is arithmetic rather than a search.
                voice.notationAt = static_cast<std::size_t>(
                                       argument.data() - whole.data())
                                   + 1;
            }
            else if (name == "s")
            {
                preset.shape = shapeIn(argument);
            }
            else if (name == "base")
            {
                preset.baseHertz = positiveIn(name, argument);
            }
            else if (name == "o")
            {
                preset.transpose +=
                    kSemitonesPerOctave * wholeIn(name, argument);
            }
            else if (name == "trans")
            {
                preset.transpose += wholeIn(name, argument);
            }
            else if (name == "gain")
            {
                preset.gain = withinOne(name, argument);
            }
            else if (name == "pan")
            {
                preset.pan = withinOne(name, argument);
            }
            else if (name == "att")
            {
                preset.attackMs = millisecondsIn(name, argument);
            }
            else if (name == "dec")
            {
                preset.decayMs = millisecondsIn(name, argument);
            }
            else if (name == "sus")
            {
                preset.sustain = withinOne(name, argument);
            }
            else if (name == "rel")
            {
                preset.releaseMs = millisecondsIn(name, argument);
            }
            else if (name == "hold")
            {
                preset.maxHoldMs = millisecondsIn(name, argument);
            }
            else if (name == "lpf")
            {
                preset.filter.mode = FilterMode::LowPass;
                preset.filter.cutoff = positiveIn(name, argument);
            }
            else if (name == "hpf")
            {
                preset.filter.mode = FilterMode::HighPass;
                preset.filter.cutoff = positiveIn(name, argument);
            }
            else if (name == "bpf")
            {
                preset.filter.mode = FilterMode::BandPass;
                preset.filter.cutoff = positiveIn(name, argument);
            }
            else if (name == "res")
            {
                preset.filter.resonance = positiveIn(name, argument);
            }
            else if (name == "slide")
            {
                preset.slide = numberIn(name, argument);
            }
            else
            {
                throw ScoreError(
                    std::string(name) + "() is not a control: "
                    + std::string(kControls));
            }
        }

        // A call is a name, a bracket, an argument and a bracket.
        void readCall(
            VoiceChain &voice,
            const std::string_view segment,
            const std::string_view whole)
        {
            const auto open = segment.find('(');

            if (open == std::string_view::npos
                || !segment.ends_with(')'))
            {
                throw ScoreError(
                    std::string(segment)
                    + " is not a call of the form name(...)");
            }

            const auto name = trimmed(segment.substr(0, open));

            const auto argument = trimmed(segment.substr(
                open + 1, segment.size() - open - 2));

            if (name.empty())
            {
                throw ScoreError("a call needs a name before its (");
            }

            applyCall(voice, name, argument, whole);
        }
    } // namespace

    std::string_view voiceControls() noexcept
    {
        return kControls;
    }

    VoiceChain parseVoiceChain(const std::string_view chain)
    {
        VoiceChain voice;

        // Offsets below are into this exact view.
        // Score::spanIn maps them on into the document.
        const auto whole = trimmed(chain);

        const auto segments = segmentsOf(whole);

        for (std::size_t at = 0; at < segments.size(); ++at)
        {
            const auto segment = trimmed(segments[at]);

            if (segment.empty())
            {
                throw ScoreError("a dot with no call after it");
            }

            // The first segment is the only one that may be a preset.
            // What tells the two apart is the bracket.
            if (at == 0 && segment.find('(') == std::string_view::npos)
            {
                const auto preset = trackFor(segment);

                if (!preset.has_value())
                {
                    throw ScoreError(
                        std::string(segment)
                        + " is no preset: bass lead bell drum");
                }

                voice.preset = trackPresets()[*preset];

                continue;
            }

            readCall(voice, segment, whole);
        }

        if (voice.notation.empty())
        {
            throw ScoreError("a voice needs an n(\"...\") to play");
        }

        return voice;
    }

} // namespace antwika::music_editor
