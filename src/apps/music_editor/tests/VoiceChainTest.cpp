#include "antwika/music_editor/VoiceChain.hpp"

#include <cstddef>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

#include <antwika/synth/Filter.hpp>
#include <antwika/synth/Waveshape.hpp>

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

    // Reads and throws the result away, so a refusal is assertable.
    void read(const std::string_view chain)
    {
        const auto refused = parseVoiceChain(chain);

        (void)refused;
    }

    // A whole chain built around one call, so a test names only it.
    [[nodiscard]] TrackPreset after(const std::string &call)
    {
        return presetOf("n(\"0\")." + call);
    }
} // namespace

TEST(VoiceChainTest, ReadsTheNotationOutOfTheNCall)
{
    const auto voice = parseVoiceChain("n(\"0 3 5\")");

    EXPECT_EQ(voice.notation, "0 3 5");
    EXPECT_EQ(voice.preset, TrackPreset{});
}

// A preset is a starting point rather than an instrument.
TEST(VoiceChainTest, OpensFromWhicheverPresetIsNamed)
{
    for (std::size_t preset = 0; preset < kPresetCount; ++preset)
    {
        const std::string chain =
            std::string(trackName(preset)) + ".n(\"0\")";

        EXPECT_EQ(presetOf(chain), trackPresets()[preset]) << chain;
    }
}

// Two lines opening `drum.` are two voices that can differ.
// Which only works because a call changes a copy.
TEST(VoiceChainTest, ACallChangesACopyAndNotThePresetItself)
{
    const auto quiet = presetOf("drum.n(\"0\").gain(.12)");

    EXPECT_FLOAT_EQ(quiet.gain, 0.12F);
    EXPECT_NE(quiet, named("drum"));

    // The preset itself is exactly what it was.
    EXPECT_EQ(named("drum"), trackPresets()[trackFor("drum").value()]);
    EXPECT_NE(named("drum").gain, 0.12F);
}

TEST(VoiceChainTest, ReadsEveryShapeItHasANameFor)
{
    EXPECT_EQ(after("s(sine)").shape, Waveshape::Sine);
    EXPECT_EQ(after("s(saw)").shape, Waveshape::Saw);
    EXPECT_EQ(after("s(square)").shape, Waveshape::Square);
    EXPECT_EQ(after("s(triangle)").shape, Waveshape::Triangle);
    EXPECT_EQ(after("s(noise)").shape, Waveshape::Noise);
}

TEST(VoiceChainTest, ReadsABaseFrequency)
{
    EXPECT_DOUBLE_EQ(after("base(440)").baseHertz, 440.0);
}

// So a line can be moved an octave without rewriting every number.
TEST(VoiceChainTest, OctavesAndSemitonesAccumulateIntoOneTranspose)
{
    EXPECT_EQ(after("o(1)").transpose, 12);
    EXPECT_EQ(after("o(-1)").transpose, -12);
    EXPECT_EQ(after("o(1).o(1)").transpose, 24);
    EXPECT_EQ(after("trans(3)").transpose, 3);
    EXPECT_EQ(after("trans(3).trans(-1)").transpose, 2);
    EXPECT_EQ(after("o(-1).trans(2)").transpose, -10);
}

TEST(VoiceChainTest, ReadsAGainAndAPan)
{
    EXPECT_FLOAT_EQ(after("gain(.2)").gain, 0.2F);
    EXPECT_FLOAT_EQ(after("pan(.5)").pan, 0.5F);
    EXPECT_FLOAT_EQ(after("pan(-1)").pan, -1.0F);
    EXPECT_FLOAT_EQ(after("gain(1)").gain, 1.0F);
}

TEST(VoiceChainTest, ReadsTheWholeEnvelope)
{
    const auto shaped =
        after("att(5).dec(60).sus(.4).rel(120).hold(900)");

    EXPECT_EQ(shaped.attackMs, 5U);
    EXPECT_EQ(shaped.decayMs, 60U);
    EXPECT_FLOAT_EQ(shaped.sustain, 0.4F);
    EXPECT_EQ(shaped.releaseMs, 120U);
    EXPECT_EQ(shaped.maxHoldMs, 900U);
}

// One call names the mode and the cutoff together.
TEST(VoiceChainTest, EachFilterCallNamesItsModeAndItsCutoff)
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

TEST(VoiceChainTest, ReadsAResonance)
{
    EXPECT_DOUBLE_EQ(after("res(.6)").filter.resonance, 0.6);
}

// The one number a voice is allowed to have below nothing.
TEST(VoiceChainTest, ReadsASlideInEitherDirection)
{
    EXPECT_DOUBLE_EQ(after("slide(-40)").slide, -40.0);
    EXPECT_DOUBLE_EQ(after("slide(120)").slide, 120.0);
    EXPECT_DOUBLE_EQ(after("slide(0)").slide, 0.0);
}

// A gain reads best with a leading dot, and from_chars wants a digit.
TEST(VoiceChainTest, ANumberMayOpenWithItsDecimalPoint)
{
    EXPECT_FLOAT_EQ(after("gain(.25)").gain, 0.25F);
    EXPECT_FLOAT_EQ(after("pan(-.4)").pan, -0.4F);
}

// The dots that join calls are found at depth zero and outside quotes.
TEST(VoiceChainTest, ADotInsideBracketsIsNotASeparator)
{
    const auto voice = parseVoiceChain("n(\"0 . 3\").gain(.5)");

    EXPECT_EQ(voice.notation, "0 . 3");
    EXPECT_FLOAT_EQ(voice.preset.gain, 0.5F);
}

// A bracket inside quotes is notation rather than nesting.
TEST(VoiceChainTest, ABracketInsideQuotesIsNotDepth)
{
    const auto voice = parseVoiceChain("drum.n(\"0(3,8)\").pan(.5)");

    EXPECT_EQ(voice.notation, "0(3,8)");
    EXPECT_FLOAT_EQ(voice.preset.pan, 0.5F);
}

TEST(VoiceChainTest, TrimsTheBlanksAroundACallAndAroundTheChain)
{
    const auto voice =
        parseVoiceChain("  drum . n(\"0\") . gain( .2 )  ");

    EXPECT_EQ(voice.notation, "0");
    EXPECT_FLOAT_EQ(voice.preset.gain, 0.2F);
    EXPECT_EQ(voice.preset.shape, named("drum").shape);
}

TEST(VoiceChainTest, RefusesAControlNoVoiceHas)
{
    EXPECT_THROW(read("n(\"0\").wobble(1)"), ScoreError);
}

TEST(VoiceChainTest, RefusesAPresetNothingIsCalled)
{
    EXPECT_THROW(read("horn.n(\"0\")"), ScoreError);
}

TEST(VoiceChainTest, RefusesAShapeItHasNoNameFor)
{
    EXPECT_THROW(read("n(\"0\").s(organ)"), ScoreError);
}

TEST(VoiceChainTest, RefusesACallWithNoNameBeforeItsBracket)
{
    EXPECT_THROW(read("(\"0\")"), ScoreError);
}

TEST(VoiceChainTest, RefusesASegmentThatIsNotACall)
{
    // No bracket at all, in a position no preset may sit in.
    EXPECT_THROW(read("n(\"0\").gain"), ScoreError);

    // A bracket that opens and never closes the segment.
    EXPECT_THROW(read("n(\"0\")x"), ScoreError);
}

TEST(VoiceChainTest, RefusesADotWithNoCallAfterIt)
{
    EXPECT_THROW(read("n(\"0\")."), ScoreError);
    EXPECT_THROW(read("drum..n(\"0\")"), ScoreError);
    EXPECT_THROW(read(""), ScoreError);
    EXPECT_THROW(read("   "), ScoreError);
}

// A chain that names a sound and never says what to play with it.
TEST(VoiceChainTest, RefusesAChainWithNoNotationInIt)
{
    EXPECT_THROW(read("drum"), ScoreError);
    EXPECT_THROW(read("gain(.5)"), ScoreError);
    EXPECT_THROW(read("n(\"\")"), ScoreError);
}

TEST(VoiceChainTest, RefusesANotationThatIsNotInQuotes)
{
    // Nothing between the brackets at all.
    EXPECT_THROW(read("n()"), ScoreError);

    // Bare words rather than a string.
    EXPECT_THROW(read("n(0 3)"), ScoreError);

    // Opened and never closed, and a stray bracket after it.
    EXPECT_THROW(read("n(\"0\"))"), ScoreError);
}

TEST(VoiceChainTest, RefusesAnArgumentThatIsNotANumber)
{
    EXPECT_THROW(read("n(\"0\").gain(x)"), ScoreError);

    // A number with something left over after it.
    EXPECT_THROW(read("n(\"0\").gain(1x)"), ScoreError);
}

TEST(VoiceChainTest, RefusesAFractionWhereAWholeNumberIsWanted)
{
    EXPECT_THROW(read("n(\"0\").o(1.5)"), ScoreError);
    EXPECT_THROW(read("n(\"0\").trans(.5)"), ScoreError);
}

TEST(VoiceChainTest, RefusesADurationBelowNothing)
{
    EXPECT_THROW(read("n(\"0\").att(-5)"), ScoreError);
    EXPECT_THROW(read("n(\"0\").hold(-1)"), ScoreError);
}

TEST(VoiceChainTest, RefusesAValueOutsideMinusOneToOne)
{
    EXPECT_THROW(read("n(\"0\").gain(2)"), ScoreError);
    EXPECT_THROW(read("n(\"0\").gain(-2)"), ScoreError);
    EXPECT_THROW(read("n(\"0\").pan(1.5)"), ScoreError);
    EXPECT_THROW(read("n(\"0\").sus(-1.5)"), ScoreError);
}

// The synth holds a sustain to zero through one.
// A negative one it accepted here used to end the run at note time.
TEST(VoiceChainTest, RefusesASustainOutsideZeroToOne)
{
    EXPECT_THROW(read("n(\"0\").sus(-.5)"), ScoreError);
    EXPECT_THROW(read("n(\"0\").sus(1.5)"), ScoreError);

    EXPECT_FLOAT_EQ(after("sus(0)").sustain, 0.0F);
    EXPECT_FLOAT_EQ(after("sus(1)").sustain, 1.0F);
}

// std::from_chars reads "nan" and "inf" as numbers.
// A NaN gain used to poison every sample of the mixer's bus.
TEST(VoiceChainTest, RefusesANumberThatIsNotFinite)
{
    EXPECT_THROW(read("n(\"0\").gain(nan)"), ScoreError);
    EXPECT_THROW(read("n(\"0\").base(inf)"), ScoreError);
    EXPECT_THROW(read("n(\"0\").sus(nan)"), ScoreError);
    EXPECT_THROW(read("n(\"0\").slide(-inf)"), ScoreError);
}

// The cast to int32 is defined only inside int32's range.
TEST(VoiceChainTest, RefusesAWholeNumberOutsideItsRange)
{
    EXPECT_THROW(read("n(\"0\").o(1e10)"), ScoreError);
    EXPECT_THROW(read("n(\"0\").trans(-1e10)"), ScoreError);
}

// Ten octaves either way covers every audible ask.
// Past the bound, 2^(transpose/12) leaves what a double can hold.
TEST(VoiceChainTest, RefusesATransposePastTenOctaves)
{
    EXPECT_THROW(read("n(\"0\").o(11)"), ScoreError);
    EXPECT_THROW(read("n(\"0\").trans(121)"), ScoreError);
    EXPECT_THROW(read("n(\"0\").trans(-121)"), ScoreError);
    EXPECT_THROW(read("n(\"0\").o(200000000)"), ScoreError);

    EXPECT_EQ(after("o(10)").transpose, 120);
    EXPECT_EQ(after("trans(-120)").transpose, -120);
}

// The drum has no pitch for a periodic wave to sound at.
// The synth would refuse the first note; this line never reads.
TEST(VoiceChainTest, RefusesAPeriodicShapeWithNoBaseToPitchFrom)
{
    EXPECT_THROW(read("drum.n(\"0\").s(sine)"), ScoreError);

    // Given a base, the same switch is an ordinary edit.
    EXPECT_DOUBLE_EQ(
        presetOf("drum.n(\"0\").s(sine).base(220)").baseHertz, 220.0);
}

// hold(0) with rel(0) is a voice of no frames at all.
TEST(VoiceChainTest, RefusesAnEnvelopeThatCouldNeverBeHeard)
{
    EXPECT_THROW(read("n(\"0\").hold(0).rel(0)"), ScoreError);

    // Either alone leaves something to hear.
    EXPECT_EQ(after("hold(0)").maxHoldMs, 0U);
    EXPECT_EQ(after("rel(0)").releaseMs, 0U);
}

TEST(VoiceChainTest, RefusesNothingWhereSomethingPositiveIsWanted)
{
    EXPECT_THROW(read("n(\"0\").base(0)"), ScoreError);
    EXPECT_THROW(read("n(\"0\").lpf(-100)"), ScoreError);
    EXPECT_THROW(read("n(\"0\").res(0)"), ScoreError);
}

// So the list a person reads cannot drift from the one that is taken.
TEST(VoiceChainTest, TheRefusalNamesEveryControlThereIs)
{
    const std::string controls{voiceControls()};

    EXPECT_FALSE(controls.empty());

    for (const auto *call :
         {"n", "s", "base", "o", "trans", "gain", "pan", "att", "dec",
          "sus", "rel", "hold", "lpf", "hpf", "bpf", "res", "slide"})
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

TEST(VoiceChainTest, ComparesFieldByField)
{
    const auto voice = parseVoiceChain("n(\"0\")");

    EXPECT_EQ(voice, parseVoiceChain("n(\"0\")"));
    EXPECT_NE(voice, parseVoiceChain("n(\"3\")"));
    EXPECT_NE(voice, parseVoiceChain("n(\"0\").gain(.9)"));
    EXPECT_EQ(VoiceChain{}, VoiceChain{});
}

// Where the notation's characters begin in the chain.
// What maps a note's span back onto the document.
TEST(VoiceChainTest, RemembersWhereTheNotationSits)
{
    const auto plain = parseVoiceChain("n(\"0 3\")");

    // n(" is three characters before the content.
    EXPECT_EQ(plain.notationAt, 3U);

    const auto chained = parseVoiceChain("drum.n(\"0(3,8)\").gain(.2)");

    EXPECT_EQ(chained.notationAt, 8U);
    EXPECT_EQ(chained.notation, "0(3,8)");
}

TEST(VoiceChainTest, ComparesChainsFieldByField)
{
    const auto chain = parseVoiceChain("drum.n(\"0\")");

    EXPECT_EQ(chain, parseVoiceChain("drum.n(\"0\")"));

    // A different preset, a different notation, a different offset.
    EXPECT_NE(chain, parseVoiceChain("bass.n(\"0\")"));
    EXPECT_NE(chain, parseVoiceChain("drum.n(\"3\")"));
    EXPECT_NE(chain, parseVoiceChain("drum .n(\"0\")"));
}
