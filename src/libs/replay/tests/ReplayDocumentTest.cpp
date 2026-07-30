#include <gtest/gtest.h>

#include <antwika/gfx/Size.hpp>

#include "antwika/replay/ReplayDocument.hpp"

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::gfx::Size;
using antwika::replay::ReplayDocument;

namespace
{
    [[nodiscard]] ReplayDocument aDocument()
    {
        return ReplayDocument{
            .events =
                {
                    TickEvent{
                        .tick = 0,
                        .event = Event{.name = "game.score_increment"},
                    },
                },
            .canvas = Size{.width = 1024, .height = 640},
        };
    }
} // namespace

TEST(ReplayDocumentTest, EqualsAnIdenticalDocument)
{
    EXPECT_EQ(aDocument(), aDocument());
}

TEST(ReplayDocumentTest, DiffersWhenTheEventsDiffer)
{
    ReplayDocument other = aDocument();
    other.events.clear();

    EXPECT_NE(aDocument(), other);
}

TEST(ReplayDocumentTest, DiffersWhenOnlyOneSideRecordedItsCanvas)
{
    ReplayDocument other = aDocument();
    other.canvas.reset();

    EXPECT_NE(aDocument(), other);
}

TEST(ReplayDocumentTest, DiffersWhenBothCanvasesAreSetButNotEqual)
{
    ReplayDocument other = aDocument();
    other.canvas = Size{.width = 800, .height = 600};

    EXPECT_NE(aDocument(), other);
}

TEST(ReplayDocumentTest, DefaultsToNoEventsAndNoCanvas)
{
    const ReplayDocument document;

    EXPECT_TRUE(document.events.empty());
    EXPECT_FALSE(document.canvas.has_value());
}
