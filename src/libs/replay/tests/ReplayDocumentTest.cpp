#include <gtest/gtest.h>

#include <antwika/geometry/Size.hpp>

#include "antwika/replay/ReplayDocument.hpp"

using antwika::event::Event;
using antwika::event::EventName;
using antwika::event::TickEvent;
using antwika::geometry::Size;
using antwika::replay::ReplayDocument;

namespace
{
    [[nodiscard]] ReplayDocument getADocument()
    {
        return ReplayDocument{
            .events =
                {
                    TickEvent{
                        .tick = 0,
                        .event = Event{.name = EventName{"game.score_increment"}},
                    },
                },
            .canvasSize = Size{.width = 1024, .height = 640},
        };
    }
}

TEST(ReplayDocumentTest, OperatorEquals_MatchesAnIdenticalDocument)
{
    EXPECT_EQ(getADocument(), getADocument());
}

TEST(ReplayDocumentTest, OperatorEquals_SeparatesDifferentEvents)
{
    ReplayDocument otherDocument = getADocument();
    otherDocument.events.clear();

    EXPECT_NE(getADocument(), otherDocument);
}

TEST(ReplayDocumentTest, OperatorEquals_SeparatesOneRecordedCanvas)
{
    ReplayDocument otherDocument = getADocument();
    otherDocument.canvasSize.reset();

    EXPECT_NE(getADocument(), otherDocument);
}

TEST(ReplayDocumentTest, OperatorEquals_SeparatesTwoUnequalCanvases)
{
    ReplayDocument otherDocument = getADocument();
    otherDocument.canvasSize = Size{.width = 800, .height = 600};

    EXPECT_NE(getADocument(), otherDocument);
}

TEST(ReplayDocumentTest, Ctor_DefaultsToNoEventsOrCanvas)
{
    const ReplayDocument document;

    EXPECT_TRUE(document.events.empty());
    EXPECT_FALSE(document.canvasSize.has_value());
}
