#include "antwika/synth/TriggerRequest.hpp"

#include <gtest/gtest.h>

#include "antwika/synth/VoiceDesc.hpp"

using antwika::synth::TriggerRequest;
using antwika::synth::VoiceDesc;

TEST(TriggerRequestTest, DefaultsToTheVeryFirstFrame)
{
    EXPECT_EQ(TriggerRequest{}.startFrame, 0U);
    EXPECT_EQ(TriggerRequest{}.voice, VoiceDesc{});
}

// "Now" is deliberately not expressible here.
// It is not expressible for antwika::sound::PlayRequest either.
TEST(TriggerRequestTest, ComparesOnTheVoiceAndTheMoment)
{
    const TriggerRequest request{
        .voice = VoiceDesc{.hold = 64}, .startFrame = 4800};

    const TriggerRequest later{
        .voice = VoiceDesc{.hold = 64}, .startFrame = 4801};

    EXPECT_EQ(request, request);
    EXPECT_NE(request, TriggerRequest{});
    EXPECT_NE(request, later);
}
