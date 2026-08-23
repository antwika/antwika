#include <gtest/gtest.h>

#include <vector>

#include "Trail.hpp"
#include "EntropyIndex.hpp"
#include "antwika/wfc/Domain.hpp"

using antwika::wfc::Domain;
using antwika::wfc::detail::EntropyIndex;
using antwika::wfc::detail::Trail;

TEST(TrailTest, Rewind_RestoresASingleRemoval)
{
    std::vector<Domain> waveDomains{Domain(3)};
    EntropyIndex entropyIndex(waveDomains, {});
    Trail trail;

    const std::size_t checkpoint = trail.getCheckpoint();
    waveDomains[0].remove(1);
    trail.record(0, 1);
    entropyIndex.update(0, waveDomains[0]);

    EXPECT_FALSE(waveDomains[0].contains(1));

    trail.rewindTo(checkpoint, waveDomains, entropyIndex);
    EXPECT_TRUE(waveDomains[0].contains(1));
    EXPECT_EQ(waveDomains[0], Domain(3));
}

TEST(TrailTest, Rewind_RestoresManyRemovalsFromACell)
{
    std::vector<Domain> waveDomains{Domain(4)};
    EntropyIndex entropyIndex(waveDomains, {});
    Trail trail;

    const std::size_t checkpoint = trail.getCheckpoint();
    waveDomains[0].restrictTo(2);
    trail.record(0, 0);
    trail.record(0, 1);
    trail.record(0, 3);
    entropyIndex.update(0, waveDomains[0]);

    EXPECT_TRUE(waveDomains[0].isSingleton());

    trail.rewindTo(checkpoint, waveDomains, entropyIndex);
    EXPECT_EQ(waveDomains[0], Domain(4));
}

TEST(TrailTest, Rewind_UndoesOnlyEntriesAfterACheckpoint)
{
    std::vector<Domain> waveDomains{Domain(3), Domain(3)};
    EntropyIndex entropyIndex(waveDomains, {});
    Trail trail;

    waveDomains[0].remove(0);
    trail.record(0, 0);

    const std::size_t checkpoint = trail.getCheckpoint();
    waveDomains[1].remove(0);
    trail.record(1, 0);
    entropyIndex.update(1, waveDomains[1]);

    trail.rewindTo(checkpoint, waveDomains, entropyIndex);

    EXPECT_FALSE(waveDomains[0].contains(0));
    EXPECT_TRUE(waveDomains[1].contains(0));
}
