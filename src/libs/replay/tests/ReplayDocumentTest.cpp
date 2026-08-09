#include <gtest/gtest.h>

#include <antwika/geometry/Size.hpp>

#include "antwika/replay/ReplayDocument.hpp"

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::geometry::Size;
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
}

TEST(ReplayDocumentTest, OperatorEquals_MatchesAnIdenticalDocument)
{
    EXPECT_EQ(aDocument(), aDocument());
}

TEST(ReplayDocumentTest, OperatorEquals_SeparatesDifferentEvents)
{
    ReplayDocument other = aDocument();
    other.events.clear();

    EXPECT_NE(aDocument(), other);
}

TEST(ReplayDocumentTest, OperatorEquals_SeparatesOneRecordedCanvas)
{
    ReplayDocument other = aDocument();
    other.canvas.reset();

    EXPECT_NE(aDocument(), other);
}

TEST(ReplayDocumentTest, OperatorEquals_SeparatesTwoUnequalCanvases)
{
    ReplayDocument other = aDocument();
    other.canvas = Size{.width = 800, .height = 600};

    EXPECT_NE(aDocument(), other);
}

TEST(ReplayDocumentTest, Ctor_DefaultsToNoEventsOrCanvas)
{
    const ReplayDocument document;

    EXPECT_TRUE(document.events.empty());
    EXPECT_FALSE(document.canvas.has_value());
}
