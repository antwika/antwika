#include <gmock/gmock.h>

#include <antwika/event/conformance/ScriptedSourceTraits.hpp>
#include <antwika/event/conformance/TickEventSourceConformanceTest.hpp>
#include <antwika/input/fakes/FakeHalvingPointerMapping.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/input/BufferedInputSource.hpp"
#include "antwika/input/CoalescingPointerSource.hpp"
#include "antwika/input/IdleMotionFilter.hpp"
#include "antwika/input/InputEventCodec.hpp"
#include "antwika/input/Key.hpp"
#include "antwika/input/LiveInputSource.hpp"
#include "antwika/input/MappedPointerSource.hpp"
#include "antwika/input/NullInputBackend.hpp"
#include "antwika/input/PointerHintChannel.hpp"
#include "antwika/input/PointerHintSource.hpp"
#include "antwika/input/StopOnKeySource.hpp"

namespace antwika::event::conformance
{

    namespace
    {

        using antwika::log::mocks::MockLogger;
        using ::testing::NiceMock;

        struct CodecDependencies
        {
            input::InputEventCodec codec;
        };

        struct LiveInputDependencies
        {
            NiceMock<MockLogger> logger;
            input::NullInputBackend backend{logger};
            input::InputEventCodec codec;
        };

        struct MappedPointerDependencies : CodecDependencies
        {
            input::fakes::FakeHalvingPointerMapping mapping;
        };

        struct PointerHintDependencies : CodecDependencies
        {
            input::PointerHintChannel channel;
        };

        using BufferedInputSourceTraits =
            ScriptedSourceTraits<input::BufferedInputSource>;

        using CoalescingPointerSourceTraits =
            ScriptedSourceTraits<input::CoalescingPointerSource>;

        struct LiveInputSourceTraits final
            : LiveInputDependencies,
              ScriptedSourceTraits<input::LiveInputSource>
        {
            LiveInputSourceTraits()
                : ScriptedSourceTraits(backend, codec)
            {
            }
        };

        struct StopOnKeySourceTraits final
            : CodecDependencies,
              ScriptedSourceTraits<input::StopOnKeySource>
        {
            StopOnKeySourceTraits()
                : ScriptedSourceTraits(codec, input::Key::Escape)
            {
            }
        };

        struct IdleMotionFilterTraits final
            : CodecDependencies,
              ScriptedSourceTraits<input::IdleMotionFilter>
        {
            IdleMotionFilterTraits()
                : ScriptedSourceTraits(codec)
            {
            }
        };

        struct MappedPointerSourceTraits final
            : MappedPointerDependencies,
              ScriptedSourceTraits<input::MappedPointerSource>
        {
            MappedPointerSourceTraits()
                : ScriptedSourceTraits(codec, mapping)
            {
            }
        };

        struct PointerHintSourceTraits final
            : PointerHintDependencies,
              ScriptedSourceTraits<input::PointerHintSource>
        {
            PointerHintSourceTraits()
                : ScriptedSourceTraits(codec, channel)
            {
            }
        };

    }

    INSTANTIATE_TYPED_TEST_SUITE_P(
        BufferedInput,
        TickEventSourceConformanceTest,
        BufferedInputSourceTraits);

    INSTANTIATE_TYPED_TEST_SUITE_P(
        LiveInput, TickEventSourceConformanceTest, LiveInputSourceTraits);

    INSTANTIATE_TYPED_TEST_SUITE_P(
        StopOnKey, TickEventSourceConformanceTest, StopOnKeySourceTraits);

    INSTANTIATE_TYPED_TEST_SUITE_P(
        CoalescingPointer,
        TickEventSourceConformanceTest,
        CoalescingPointerSourceTraits);

    INSTANTIATE_TYPED_TEST_SUITE_P(
        IdleMotion, TickEventSourceConformanceTest, IdleMotionFilterTraits);

    INSTANTIATE_TYPED_TEST_SUITE_P(
        MappedPointer,
        TickEventSourceConformanceTest,
        MappedPointerSourceTraits);

    INSTANTIATE_TYPED_TEST_SUITE_P(
        PointerHint,
        TickEventSourceConformanceTest,
        PointerHintSourceTraits);

}
