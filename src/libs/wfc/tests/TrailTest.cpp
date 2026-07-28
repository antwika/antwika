#include "Trail.hpp"

#include <vector>

#include <gtest/gtest.h>

#include "EntropyIndex.hpp"
#include "antwika/wfc/Domain.hpp"

using antwika::wfc::Domain;
using antwika::wfc::detail::EntropyIndex;
using antwika::wfc::detail::Trail;

TEST(TrailTest, RewindRestoresSingleRemoval)
{
    std::vector<Domain> wave{Domain(3)};
    EntropyIndex entropyIndex(wave, {});
    Trail trail;

    const std::size_t checkpoint = trail.checkpoint();
    wave[0].remove(1);
    trail.record(0, 1);
    entropyIndex.update(0, wave[0]);

    EXPECT_FALSE(wave[0].contains(1));

    trail.rewindTo(checkpoint, wave, entropyIndex);
    EXPECT_TRUE(wave[0].contains(1));
    EXPECT_EQ(wave[0], Domain(3));
}

TEST(TrailTest, RewindRestoresMultipleRemovalsFromSameCell)
{
    std::vector<Domain> wave{Domain(4)};
    EntropyIndex entropyIndex(wave, {});
    Trail trail;

    const std::size_t checkpoint = trail.checkpoint();
    wave[0].restrictTo(2);
    trail.record(0, 0);
    trail.record(0, 1);
    trail.record(0, 3);
    entropyIndex.update(0, wave[0]);

    EXPECT_TRUE(wave[0].isSingleton());

    trail.rewindTo(checkpoint, wave, entropyIndex);
    EXPECT_EQ(wave[0], Domain(4));
}

TEST(TrailTest, RewindOnlyUndoesEntriesAfterCheckpoint)
{
    std::vector<Domain> wave{Domain(3), Domain(3)};
    EntropyIndex entropyIndex(wave, {});
    Trail trail;

    wave[0].remove(0);
    trail.record(0, 0);

    const std::size_t checkpoint = trail.checkpoint();
    wave[1].remove(0);
    trail.record(1, 0);
    entropyIndex.update(1, wave[1]);

    trail.rewindTo(checkpoint, wave, entropyIndex);

    EXPECT_FALSE(wave[0].contains(0));
    EXPECT_TRUE(wave[1].contains(0));
}
