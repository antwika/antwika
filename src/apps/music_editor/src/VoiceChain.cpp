#include "antwika/music_editor/VoiceChain.hpp"

#include <array>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include <antwika/synth/Filter.hpp>
#include <antwika/synth/Waveshape.hpp>

#include "antwika/music_editor/ScoreError.hpp"

#include "ScoreText.hpp"

namespace antwika::music_editor
{

    namespace
    {
        using antwika::synth::FilterMode;
        using antwika::synth::Waveshape;

        constexpr std::string_view kControls{
            "n s base o trans gain pan att dec sus rel hold lpf hpf "
            "bpf res slide vib vibdepth arp delay delaymix harm "
            "pianoroll waveform"};

        constexpr std::array<std::string_view, 5> kShapes{
            "sine", "saw", "square", "triangle", "noise"};

        using detail::kSemitonesPerOctave;
        using detail::trimmed;

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

        } // GCOVR_EXCL_LINE

        [[nodiscard]] double numberIn(
            std::string_view name, std::string_view argument)
        {
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

            if (!std::isfinite(value))
            {
                throw ScoreError(
                    std::string(name) + "(" + std::string(argument)
                    + ") wants a finite number");
            }

            return value;
        }

        [[nodiscard]] std::int32_t wholeIn(
            std::string_view name, std::string_view argument)
        {
            const auto value = numberIn(name, argument);

            constexpr auto kLow = static_cast<double>(
                std::numeric_limits<std::int32_t>::min());
            constexpr auto kHigh = static_cast<double>(
                std::numeric_limits<std::int32_t>::max());

            if (value < kLow || value > kHigh)
            {
                throw ScoreError(
                    std::string(name) + "(" + std::string(argument)
                    + ") wants a whole number");
            }

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

        [[nodiscard]] float zeroToOne(
            std::string_view name, std::string_view argument)
        {
            const auto value = numberIn(name, argument);

            if (value < 0.0 || value > 1.0)
            {
                throw ScoreError(
                    std::string(name) + " lies between 0 and 1");
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

        constexpr std::int64_t kMaxTranspose = 120;

        void addTranspose(
            TrackPreset &preset,
            const std::string_view name,
            const std::int64_t amount)
        {
            const auto total =
                static_cast<std::int64_t>(preset.transpose) + amount;

            if (total < -kMaxTranspose || total > kMaxTranspose)
            {
                throw ScoreError(
                    std::string(name) + " transposes past "
                    + std::to_string(kMaxTranspose) + " semitones");
            }

            preset.transpose = static_cast<std::int32_t>(total);
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
                addTranspose(
                    preset, name,
                    static_cast<std::int64_t>(kSemitonesPerOctave)
                        * wholeIn(name, argument));
            }
            else if (name == "trans")
            {
                addTranspose(preset, name, wholeIn(name, argument));
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
                preset.sustain = zeroToOne(name, argument);
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
            else if (name == "vib")
            {
                preset.vibratoHertz = positiveIn(name, argument);
            }
            else if (name == "vibdepth")
            {
                preset.vibratoDepth = zeroToOne(name, argument);
            }
            else if (name == "arp")
            {
                preset.arpSemitones = static_cast<std::int32_t>(
                    wholeIn(name, argument));
            }
            else if (name == "delay")
            {
                preset.delayMs = millisecondsIn(name, argument);
            }
            else if (name == "delaymix")
            {
                preset.delayMix = zeroToOne(name, argument);
            }
            else if (name == "harm")
            {
                preset.harmonySemitones = static_cast<std::int32_t>(
                    wholeIn(name, argument));
            }
            else if (name == "pianoroll" || name == "waveform")
            {
                if (!argument.empty())
                {
                    throw ScoreError(
                        std::string(name)
                        + "() takes nothing in its brackets");
                }

                if (name == "pianoroll")
                {
                    voice.pianoroll = true;
                }
                else
                {
                    voice.waveform = true;
                }
            }
            else
            {
                throw ScoreError(
                    std::string(name) + "() is not a control: "
                    + std::string(kControls));
            }
        }

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
    }

    std::string_view voiceControls() noexcept
    {
        return kControls;
    }

    VoiceChain parseVoiceChain(const std::string_view chain)
    {
        VoiceChain voice;

        const auto whole = trimmed(chain);

        const auto segments = segmentsOf(whole);

        for (std::size_t at = 0; at < segments.size(); ++at)
        {
            const auto segment = trimmed(segments[at]);

            if (segment.empty())
            {
                throw ScoreError("a dot with no call after it");
            }

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

        if (synth::isPeriodic(voice.preset.shape)
            && !(voice.preset.baseHertz > 0.0))
        {
            throw ScoreError(
                "s(" + std::string(waveshapeName(voice.preset.shape))
                + ") wants a base(...) above zero to pitch from");
        }

        if (voice.preset.maxHoldMs == 0 && voice.preset.releaseMs == 0)
        {
            throw ScoreError(
                "hold(0) with rel(0) would never be heard");
        }

        return voice;
    }

}
