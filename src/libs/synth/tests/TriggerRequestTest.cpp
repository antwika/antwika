#include <gtest/gtest.h>

#include "antwika/synth/TriggerRequest.hpp"
#include "antwika/synth/VoiceDesc.hpp"

using antwika::synth::TriggerRequest;
using antwika::synth::VoiceDesc;

TEST(TriggerRequestTest, Ctor_DefaultsToTheVeryFirstFrame)
{
    EXPECT_EQ(TriggerRequest{}.startFrame, 0U);
    EXPECT_EQ(TriggerRequest{}.voice, VoiceDesc{});
}

TEST(TriggerRequestTest, OperatorEquals_ComparesTheVoiceAndTheMoment)
{
    const TriggerRequest request{
        .voice = VoiceDesc{.hold = 64}, .startFrame = 4800};

    const TriggerRequest later{
        .voice = VoiceDesc{.hold = 64}, .startFrame = 4801};

    const auto twin = request;
    EXPECT_EQ(request, twin);
    EXPECT_NE(request, TriggerRequest{});
    EXPECT_NE(request, later);
}
