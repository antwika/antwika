#include <gtest/gtest.h>

#include <cstddef>
#include <string>
#include <string_view>

#include <antwika/synth/Filter.hpp>
#include <antwika/synth/Waveshape.hpp>

#include "antwika/music_editor/VoiceChain.hpp"
#include "antwika/music_editor/ScoreError.hpp"
#include "antwika/music_editor/TrackPreset.hpp"

using antwika::music_editor::kPresetCount;
using antwika::music_editor::parseVoiceChain;
using antwika::music_editor::ScoreError;
using antwika::music_editor::trackFor;
using antwika::music_editor::trackName;
using antwika::music_editor::TrackPreset;
using antwika::music_editor::trackPresets;
using antwika::music_editor::VoiceChain;
using antwika::music_editor::voiceControls;
using antwika::synth::FilterMode;
using antwika::synth::Waveshape;

namespace
{
    [[nodiscard]] TrackPreset presetOf(const std::string_view chain)
    {
        return parseVoiceChain(chain).preset;
    }

    [[nodiscard]] TrackPreset named(const std::string_view name)
    {
        return trackPresets()[trackFor(name).value()];
    }

    void read(const std::string_view chain)
    {
        const auto refused = parseVoiceChain(chain);

        (void)refused;
    }

    [[nodiscard]] TrackPreset after(const std::string &call)
    {
        return presetOf("n(\"0\")." + call);
    }
}

TEST(VoiceChainTest, ParseVoiceChain_ReadsTheNotationOutOfTheNCall)
{
    const auto voice = parseVoiceChain("n(\"0 3 5\")");

    EXPECT_EQ(voice.notation, "0 3 5");
    EXPECT_EQ(voice.preset, TrackPreset{});
}

TEST(VoiceChainTest, ParseVoiceChain_OpensFromWhicheverPresetIsNamed)
{
    for (std::size_t preset = 0; preset < kPresetCount; ++preset)
    {
        const std::string chain =
            std::string(trackName(preset)) + ".n(\"0\")";

        EXPECT_EQ(presetOf(chain), trackPresets()[preset]) << chain;
    }
}

TEST(VoiceChainTest, ParseVoiceChain_LeavesThePresetAlone)
{
    const auto quiet = presetOf("drum.n(\"0\").gain(.12)");

    EXPECT_FLOAT_EQ(quiet.gain, 0.12F);
    EXPECT_NE(quiet, named("drum"));

    EXPECT_EQ(named("drum"), trackPresets()[trackFor("drum").value()]);
    EXPECT_NE(named("drum").gain, 0.12F);
}

TEST(VoiceChainTest, ParseVoiceChain_ReadsEveryShapeItHasANameFor)
{
    EXPECT_EQ(after("s(sine)").shape, Waveshape::Sine);
    EXPECT_EQ(after("s(saw)").shape, Waveshape::Saw);
    EXPECT_EQ(after("s(square)").shape, Waveshape::Square);
    EXPECT_EQ(after("s(triangle)").shape, Waveshape::Triangle);
    EXPECT_EQ(after("s(noise)").shape, Waveshape::Noise);
}

TEST(VoiceChainTest, ParseVoiceChain_ReadsABaseFrequency)
{
    EXPECT_DOUBLE_EQ(after("base(440)").baseHertz, 440.0);
}

TEST(VoiceChainTest, ParseVoiceChain_AccumulatesOneTranspose)
{
    EXPECT_EQ(after("o(1)").transpose, 12);
    EXPECT_EQ(after("o(-1)").transpose, -12);
    EXPECT_EQ(after("o(1).o(1)").transpose, 24);
    EXPECT_EQ(after("trans(3)").transpose, 3);
    EXPECT_EQ(after("trans(3).trans(-1)").transpose, 2);
    EXPECT_EQ(after("o(-1).trans(2)").transpose, -10);
}

TEST(VoiceChainTest, ParseVoiceChain_ReadsAGainAndAPan)
{
    EXPECT_FLOAT_EQ(after("gain(.2)").gain, 0.2F);
    EXPECT_FLOAT_EQ(after("pan(.5)").pan, 0.5F);
    EXPECT_FLOAT_EQ(after("pan(-1)").pan, -1.0F);
    EXPECT_FLOAT_EQ(after("gain(1)").gain, 1.0F);
}

TEST(VoiceChainTest, ParseVoiceChain_ReadsTheWholeEnvelope)
{
    const auto shaped =
        after("att(5).dec(60).sus(.4).rel(120).hold(900)");

    EXPECT_EQ(shaped.attackMs, 5U);
    EXPECT_EQ(shaped.decayMs, 60U);
    EXPECT_FLOAT_EQ(shaped.sustain, 0.4F);
    EXPECT_EQ(shaped.releaseMs, 120U);
    EXPECT_EQ(shaped.maxHoldMs, 900U);
}

TEST(VoiceChainTest, ParseVoiceChain_ReadsAFilterModeAndCutoff)
{
    const auto low = after("lpf(2500)");
    EXPECT_EQ(low.filter.mode, FilterMode::LowPass);
    EXPECT_DOUBLE_EQ(low.filter.cutoff, 2500.0);

    const auto high = after("hpf(4000)");
    EXPECT_EQ(high.filter.mode, FilterMode::HighPass);
    EXPECT_DOUBLE_EQ(high.filter.cutoff, 4000.0);

    const auto band = after("bpf(800)");
    EXPECT_EQ(band.filter.mode, FilterMode::BandPass);
    EXPECT_DOUBLE_EQ(band.filter.cutoff, 800.0);
}

TEST(VoiceChainTest, ParseVoiceChain_ReadsAResonance)
{
    EXPECT_DOUBLE_EQ(after("res(.6)").filter.resonance, 0.6);
}

TEST(VoiceChainTest, ParseVoiceChain_ReadsASlideInEitherDirection)
{
    EXPECT_DOUBLE_EQ(after("slide(-40)").slide, -40.0);
    EXPECT_DOUBLE_EQ(after("slide(120)").slide, 120.0);
    EXPECT_DOUBLE_EQ(after("slide(0)").slide, 0.0);
}

TEST(VoiceChainTest, ParseVoiceChain_AllowsALeadingDecimalPoint)
{
    EXPECT_FLOAT_EQ(after("gain(.25)").gain, 0.25F);
    EXPECT_FLOAT_EQ(after("pan(-.4)").pan, -0.4F);
}

TEST(VoiceChainTest, ParseVoiceChain_KeepsADotInsideBrackets)
{
    const auto voice = parseVoiceChain("n(\"0 . 3\").gain(.5)");

    EXPECT_EQ(voice.notation, "0 . 3");
    EXPECT_FLOAT_EQ(voice.preset.gain, 0.5F);
}

TEST(VoiceChainTest, ParseVoiceChain_KeepsABracketInsideQuotes)
{
    const auto voice = parseVoiceChain("drum.n(\"0(3,8)\").pan(.5)");

    EXPECT_EQ(voice.notation, "0(3,8)");
    EXPECT_FLOAT_EQ(voice.preset.pan, 0.5F);
}

TEST(VoiceChainTest, ParseVoiceChain_TrimsBlanksAroundACall)
{
    const auto voice =
        parseVoiceChain("  drum . n(\"0\") . gain( .2 )  ");

    EXPECT_EQ(voice.notation, "0");
    EXPECT_FLOAT_EQ(voice.preset.gain, 0.2F);
    EXPECT_EQ(voice.preset.shape, named("drum").shape);
}

TEST(VoiceChainTest, ParseVoiceChain_RefusesAnUnknownControl)
{
    EXPECT_THROW(read("n(\"0\").wobble(1)"), ScoreError);
}

TEST(VoiceChainTest, ParseVoiceChain_RefusesAPresetNothingIsCalled)
{
    EXPECT_THROW(read("horn.n(\"0\")"), ScoreError);
}

TEST(VoiceChainTest, ParseVoiceChain_RefusesAShapeItHasNoNameFor)
{
    EXPECT_THROW(read("n(\"0\").s(organ)"), ScoreError);
}

TEST(VoiceChainTest, ParseVoiceChain_RefusesANamelessCall)
{
    EXPECT_THROW(read("(\"0\")"), ScoreError);
}

TEST(VoiceChainTest, ParseVoiceChain_RefusesASegmentThatIsNotACall)
{
    EXPECT_THROW(read("n(\"0\").gain"), ScoreError);

    EXPECT_THROW(read("n(\"0\")x"), ScoreError);
}

TEST(VoiceChainTest, ParseVoiceChain_RefusesADotWithNoCallAfterIt)
{
    EXPECT_THROW(read("n(\"0\")."), ScoreError);
    EXPECT_THROW(read("drum..n(\"0\")"), ScoreError);
    EXPECT_THROW(read(""), ScoreError);
    EXPECT_THROW(read("   "), ScoreError);
}

TEST(VoiceChainTest, ParseVoiceChain_RefusesAChainWithNoNotationInIt)
{
    EXPECT_THROW(read("drum"), ScoreError);
    EXPECT_THROW(read("gain(.5)"), ScoreError);
    EXPECT_THROW(read("n(\"\")"), ScoreError);
}

TEST(VoiceChainTest, ParseVoiceChain_RefusesANotationThatIsNotInQuotes)
{
    EXPECT_THROW(read("n()"), ScoreError);

    EXPECT_THROW(read("n(0 3)"), ScoreError);

    EXPECT_THROW(read("n(\"0\"))"), ScoreError);
}

TEST(VoiceChainTest, ParseVoiceChain_RefusesAnArgumentThatIsNotANumber)
{
    EXPECT_THROW(read("n(\"0\").gain(x)"), ScoreError);

    EXPECT_THROW(read("n(\"0\").gain(1x)"), ScoreError);
}

TEST(VoiceChainTest, ParseVoiceChain_RefusesAFractionForAWhole)
{
    EXPECT_THROW(read("n(\"0\").o(1.5)"), ScoreError);
    EXPECT_THROW(read("n(\"0\").trans(.5)"), ScoreError);
}

TEST(VoiceChainTest, ParseVoiceChain_RefusesADurationBelowNothing)
{
    EXPECT_THROW(read("n(\"0\").att(-5)"), ScoreError);
    EXPECT_THROW(read("n(\"0\").hold(-1)"), ScoreError);
}

TEST(VoiceChainTest, ParseVoiceChain_RefusesAValueOutsideOne)
{
    EXPECT_THROW(read("n(\"0\").gain(2)"), ScoreError);
    EXPECT_THROW(read("n(\"0\").gain(-2)"), ScoreError);
    EXPECT_THROW(read("n(\"0\").pan(1.5)"), ScoreError);
    EXPECT_THROW(read("n(\"0\").sus(-1.5)"), ScoreError);
}

TEST(VoiceChainTest, ParseVoiceChain_RefusesASustainOutsideZeroToOne)
{
    EXPECT_THROW(read("n(\"0\").sus(-.5)"), ScoreError);
    EXPECT_THROW(read("n(\"0\").sus(1.5)"), ScoreError);

    EXPECT_FLOAT_EQ(after("sus(0)").sustain, 0.0F);
    EXPECT_FLOAT_EQ(after("sus(1)").sustain, 1.0F);
}

TEST(VoiceChainTest, ParseVoiceChain_RefusesANumberThatIsNotFinite)
{
    EXPECT_THROW(read("n(\"0\").gain(nan)"), ScoreError);
    EXPECT_THROW(read("n(\"0\").base(inf)"), ScoreError);
    EXPECT_THROW(read("n(\"0\").sus(nan)"), ScoreError);
    EXPECT_THROW(read("n(\"0\").slide(-inf)"), ScoreError);
}

TEST(VoiceChainTest, ParseVoiceChain_RefusesAWholeOutOfRange)
{
    EXPECT_THROW(read("n(\"0\").o(1e10)"), ScoreError);
    EXPECT_THROW(read("n(\"0\").trans(-1e10)"), ScoreError);
}

TEST(VoiceChainTest, ParseVoiceChain_RefusesATransposePastTenOctaves)
{
    EXPECT_THROW(read("n(\"0\").o(11)"), ScoreError);
    EXPECT_THROW(read("n(\"0\").trans(121)"), ScoreError);
    EXPECT_THROW(read("n(\"0\").trans(-121)"), ScoreError);
    EXPECT_THROW(read("n(\"0\").o(200000000)"), ScoreError);

    EXPECT_EQ(after("o(10)").transpose, 120);
    EXPECT_EQ(after("trans(-120)").transpose, -120);
}

TEST(VoiceChainTest, ParseVoiceChain_RefusesAShapeWithNoBase)
{
    EXPECT_THROW(read("drum.n(\"0\").s(sine)"), ScoreError);

    EXPECT_DOUBLE_EQ(
        presetOf("drum.n(\"0\").s(sine).base(220)").baseHertz, 220.0);
}

TEST(VoiceChainTest, ParseVoiceChain_RefusesAnInaudibleEnvelope)
{
    EXPECT_THROW(read("n(\"0\").hold(0).rel(0)"), ScoreError);

    EXPECT_EQ(after("hold(0)").maxHoldMs, 0U);
    EXPECT_EQ(after("rel(0)").releaseMs, 0U);
}

TEST(VoiceChainTest, ParseVoiceChain_RefusesANonPositive)
{
    EXPECT_THROW(read("n(\"0\").base(0)"), ScoreError);
    EXPECT_THROW(read("n(\"0\").lpf(-100)"), ScoreError);
    EXPECT_THROW(read("n(\"0\").res(0)"), ScoreError);
}

TEST(VoiceChainTest, ParseVoiceChain_NamesEveryControlOnRefusal)
{
    const std::string controls{voiceControls()};

    EXPECT_FALSE(controls.empty());

    for (const auto *call :
         {"n", "s", "base", "o", "trans", "gain", "pan", "att", "dec",
          "sus", "rel", "hold", "lpf", "hpf", "bpf", "res", "slide",
          "vib", "vibdepth", "arp", "delay", "delaymix", "harm",
          "pianoroll", "waveform"})
    {
        EXPECT_NE(controls.find(call), std::string::npos) << call;
    }

    try
    {
        read("n(\"0\").wobble(1)");

        FAIL() << "wobble() is not a control";
    }
    catch (const ScoreError &refused)
    {
        EXPECT_NE(
            std::string(refused.what()).find(controls),
            std::string::npos);
    }
}

TEST(VoiceChainTest, ParseVoiceChain_ReadsAPianorollRequest)
{
    const auto voice = parseVoiceChain("drum.n(\"0\").pianoroll()");

    EXPECT_TRUE(voice.pianoroll);
    EXPECT_EQ(voice.preset, named("drum"));

    EXPECT_FALSE(parseVoiceChain("drum.n(\"0\")").pianoroll);
}

TEST(VoiceChainTest, ParseVoiceChain_ReadsAWaveformRequest)
{
    const auto voice = parseVoiceChain("drum.n(\"0\").waveform()");

    EXPECT_TRUE(voice.waveform);
    EXPECT_FALSE(voice.pianoroll);
    EXPECT_EQ(voice.preset, named("drum"));
}

TEST(VoiceChainTest, ParseVoiceChain_RefusesAPictureCallGivenAnything)
{
    EXPECT_THROW(read("n(\"0\").pianoroll(1)"), ScoreError);
    EXPECT_THROW(read("n(\"0\").waveform(1)"), ScoreError);
}

TEST(VoiceChainTest, OperatorEquals_ComparesFieldByField)
{
    const auto voice = parseVoiceChain("n(\"0\")");

    EXPECT_EQ(voice, parseVoiceChain("n(\"0\")"));
    EXPECT_NE(voice, parseVoiceChain("n(\"3\")"));
    EXPECT_NE(voice, parseVoiceChain("n(\"0\").gain(.9)"));
    EXPECT_NE(voice, parseVoiceChain("n(\"0\").pianoroll()"));
    EXPECT_NE(voice, parseVoiceChain("n(\"0\").waveform()"));
    EXPECT_EQ(VoiceChain{}, VoiceChain{});
}

TEST(VoiceChainTest, ParseVoiceChain_RemembersTheNotationsPlace)
{
    const auto plain = parseVoiceChain("n(\"0 3\")");

    EXPECT_EQ(plain.notationAt, 3U);

    const auto chained = parseVoiceChain("drum.n(\"0(3,8)\").gain(.2)");

    EXPECT_EQ(chained.notationAt, 8U);
    EXPECT_EQ(chained.notation, "0(3,8)");
}

TEST(VoiceChainTest, OperatorEquals_ComparesChainsFieldByField)
{
    const auto chain = parseVoiceChain("drum.n(\"0\")");

    EXPECT_EQ(chain, parseVoiceChain("drum.n(\"0\")"));

    EXPECT_NE(chain, parseVoiceChain("bass.n(\"0\")"));
    EXPECT_NE(chain, parseVoiceChain("drum.n(\"3\")"));
    EXPECT_NE(chain, parseVoiceChain("drum .n(\"0\")"));
}

TEST(VoiceChainTest, ParseVoiceChain_ReadsTheVibratoRateAndDepth)
{
    EXPECT_DOUBLE_EQ(after("vib(6)").vibratoHertz, 6.0);
    EXPECT_DOUBLE_EQ(after("vib(.5)").vibratoHertz, 0.5);
    EXPECT_FLOAT_EQ(after("vibdepth(.02)").vibratoDepth, 0.02F);
    EXPECT_FLOAT_EQ(after("vibdepth(0)").vibratoDepth, 0.0F);
}

TEST(VoiceChainTest, ParseVoiceChain_RefusesAVibratoThatIsNotPositive)
{
    EXPECT_THROW(read("n(\"0\").vib(0)"), ScoreError);
    EXPECT_THROW(read("n(\"0\").vib(-2)"), ScoreError);
    EXPECT_THROW(read("n(\"0\").vibdepth(2)"), ScoreError);
}

TEST(VoiceChainTest, ParseVoiceChain_ReadsTheArpeggioInterval)
{
    EXPECT_EQ(after("arp(12)").arpSemitones, 12);
    EXPECT_EQ(after("arp(-7)").arpSemitones, -7);
    EXPECT_EQ(after("arp(0)").arpSemitones, 0);
}

TEST(VoiceChainTest, ParseVoiceChain_RefusesAnArpeggioThatIsNotWhole)
{
    EXPECT_THROW(read("n(\"0\").arp(1.5)"), ScoreError);
}

TEST(VoiceChainTest, ParseVoiceChain_ReadsTheDelayAndItsMix)
{
    EXPECT_EQ(after("delay(250)").delayMs, 250U);
    EXPECT_FLOAT_EQ(after("delaymix(.3)").delayMix, 0.3F);
}

TEST(VoiceChainTest, ParseVoiceChain_RefusesADelayMixOutsideZeroToOne)
{
    EXPECT_THROW(read("n(\"0\").delaymix(1.5)"), ScoreError);
    EXPECT_THROW(read("n(\"0\").delay(-10)"), ScoreError);
}

TEST(VoiceChainTest, ParseVoiceChain_ReadsTheHarmonyInterval)
{
    EXPECT_EQ(after("harm(7)").harmonySemitones, 7);
    EXPECT_EQ(after("harm(-12)").harmonySemitones, -12);
}

TEST(VoiceChainTest, ParseVoiceChain_RefusesAHarmonyThatIsNotWhole)
{
    EXPECT_THROW(read("n(\"0\").harm(.5)"), ScoreError);
}
