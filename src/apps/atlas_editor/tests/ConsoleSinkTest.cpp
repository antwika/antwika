#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <antwika/console/ConsolePicture.hpp>
#include <antwika/console/ConsoleState.hpp>
#include <antwika/console/SnapshotFormat.hpp>
#include <antwika/console/conformance/ConsoleContractTest.hpp>
#include <antwika/console/conformance/ConsoleSnapshotRoundTripTest.hpp>
#include <antwika/console/testing/ConsoleScript.hpp>
#include <antwika/event/mocks/MockEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/testing/ScratchPath.hpp>
#include <antwika/ui/Pointer.hpp>

#include "antwika/atlas_editor/AtlasEditor.hpp"
#include "antwika/atlas_editor/Canvas.hpp"
#include "antwika/atlas_editor/EditorState.hpp"
#include "antwika/atlas_editor/EditorUi.hpp"
#include "antwika/atlas_editor/AtlasMeta.hpp"
#include "antwika/atlas_editor/IAtlasStore.hpp"
#include "antwika/atlas_editor/Messages.hpp"
#include "antwika/atlas_editor/OpeningSheet.hpp"
#include "antwika/atlas_editor/StateDump.hpp"
#include "antwika/atlas_editor/TileGrid.hpp"
#include "antwika/atlas_editor/Tool.hpp"
#include "antwika/atlas_editor/UiOverlay.hpp"

using antwika::atlas_editor::bootstrap;
using antwika::atlas_editor::Canvas;
using antwika::atlas_editor::describeEditor;
using antwika::atlas_editor::EditorState;
using antwika::atlas_editor::EditorSummary;
using antwika::atlas_editor::EditorWiring;
using antwika::atlas_editor::IAtlasStore;
using antwika::atlas_editor::TileGrid;
using antwika::atlas_editor::Tool;
using antwika::atlas_editor::UiOverlay;
using antwika::console::kConsoleAnimTicks;
using antwika::console::testing::keyAt;
using antwika::console::testing::kOpenTick;
using antwika::console::testing::moveTo;
using antwika::console::testing::pressAt;
using antwika::console::testing::releaseAt;
using antwika::console::testing::stopAt;
using antwika::console::testing::typeText;
using antwika::event::mocks::MockEventSink;
using antwika::event::TickEvent;
using antwika::gfx::Bitmap;
using antwika::gfx::Point;
using antwika::gfx::Size;
using antwika::input::InputEventCodec;
using antwika::input::Key;
using antwika::input::KeyPressed;
using antwika::log::mocks::MockLogger;
using antwika::replay::ReplaySource;
using antwika::testing::ScratchDirectory;
using antwika::time::Tick;
using ::testing::NiceMock;

namespace
{
    constexpr Size kCanvas{.width = 800, .height = 480};

    [[nodiscard]] TickEvent chordAt(
        const InputEventCodec &codec, const Tick tick, const Key key)
    {
        return keyAt(
            codec,
            tick,
            KeyPressed{.key = key, .modifiers = {.control = true}});
    }

    class FakeMemoryStore final : public IAtlasStore
    {
    public:
        std::optional<Bitmap> load() override
        {
            return std::nullopt;
        }

        void save(const Bitmap &image) override
        {
            written = image;
        }

        [[nodiscard]] std::optional<Bitmap> loadFrom(
            const std::string &) override
        {
            return std::nullopt;
        }

        void saveTo(const Bitmap &image, const std::string &) override
        {
            written = image;
        }

        std::optional<antwika::atlas_editor::AtlasMeta> describes{};
        std::optional<antwika::atlas_editor::AtlasMeta> metaWritten{};
        std::optional<std::string> metaWrote{};

        [[nodiscard]] std::optional<antwika::atlas_editor::AtlasMeta>
        loadMetaFrom(const std::string &) override
        {
            return describes;
        }

        void saveMetaTo(
            const antwika::atlas_editor::AtlasMeta &meta,
            const std::string &path) override
        {
            metaWrote = path;
            metaWritten = meta;
        }

        [[nodiscard]] std::string savePath() const override
        {
            return "memory.png";
        }

        std::optional<Bitmap> written{};
    };

    constexpr antwika::atlas_editor::Translator kTranslator{
        antwika::i18n::kDefaultLocale};

    [[nodiscard]] Point selectToolButton()
    {
        const EditorState state{
            Canvas::blank(kCanvas), TileGrid{}, kCanvas};
        const auto rect =
            describeEditor(state, antwika::ui::Pointer{}, kTranslator)
                .rects.find(
                    antwika::atlas_editor::widgets::toolWidget(
                        Tool::Select));

        const auto found = rect.value_or(antwika::gfx::Rect{});

        return Point{
            .x = found.origin.x
                 + static_cast<std::int32_t>(found.size.width / 2),
            .y = found.origin.y
                 + static_cast<std::int32_t>(found.size.height / 2)};
    }

    struct ConsoleHarness final
    {
        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> eventSink;
        InputEventCodec codec;
        FakeMemoryStore store;
        antwika::console::ConsolePicture consoleOverlay{kCanvas};

        EditorSummary run(
            ReplaySource &source,
            const Tick maxTicks,
            const std::string &dumpPath,
            const bool loadEnabled = true)
        {
            return bootstrap(EditorWiring{
                .logger = logger,
                .eventSink = eventSink,
                .inputSource = source,
                .codec = codec,
                .store = store,
                .translator = kTranslator,
                .canvas = kCanvas,
                .blank = kCanvas,
                .tiles = TileGrid{},
                .maxTicks = maxTicks,
                .consoleOverlay = consoleOverlay,
                .consoleLoadEnabled = loadEnabled,
                .stateDumpPath = dumpPath});
        }
    };

    [[nodiscard]] antwika::console::Snapshot readDump(
        const std::string &path)
    {
        const antwika::console::SnapshotFormat format(
            {.magic = antwika::atlas_editor::kStateDumpMagic,
             .version = antwika::atlas_editor::kStateDumpVersion},
            "antwika atlas editor state dump document",
            antwika::atlas_editor::standardStateDumpMigrations);

        return format.read(path);
    }

    struct AtlasEditorConsoleTraits final
    {
        using Summary = EditorSummary;

        static Summary run(
            std::vector<TickEvent> script,
            const std::string &dumpPath,
            const bool loadEnabled)
        {
            script.push_back(stopAt(kOpenTick + 1));

            ReplaySource source(std::move(script));
            ConsoleHarness harness;

            return harness.run(source, 40, dumpPath, loadEnabled);
        }

        static const std::vector<std::string> &console(
            const Summary &summary)
        {
            return summary.console;
        }

        static void expectUntouched(const Summary &summary)
        {
            EXPECT_EQ(summary.edits, 0U);
        }

        static std::string scratchPrefix()
        {
            return "antwika_atlas_editor_console.";
        }
    };
}

namespace antwika::console::conformance
{

    INSTANTIATE_TYPED_TEST_SUITE_P(
        AtlasEditor, ConsoleContractTest, AtlasEditorConsoleTraits);

    INSTANTIATE_TYPED_TEST_SUITE_P(
        AtlasEditor,
        ConsoleSnapshotRoundTripTest,
        AtlasEditorConsoleTraits);

}

TEST(ConsoleSinkTest, Run_PaintsNoPixelUnderTheSheet)
{
    ConsoleHarness harness;

    const Point under{.x = 120, .y = 230};
    const Point below{.x = 120, .y = 300};

    std::vector<TickEvent> events{keyAt(harness.codec, 1, Key::Grave)};
    events.push_back(pressAt(harness.codec, kOpenTick, below));
    events.push_back(releaseAt(harness.codec, kOpenTick, below));
    events.push_back(pressAt(harness.codec, kOpenTick, under));
    events.push_back(releaseAt(harness.codec, kOpenTick, under));
    events.push_back(stopAt(kOpenTick + 1));
    ReplaySource source(std::move(events));

    const auto summary = harness.run(source, 40, "unused.json");

    EXPECT_EQ(summary.edits, 1U);
}

TEST(ConsoleSinkTest, Run_KeepsChordsFromTheOpenConsole)
{
    ConsoleHarness harness;
    const Point tool = selectToolButton();

    std::vector<TickEvent> events;

    events.push_back(pressAt(harness.codec, 2, {.x = 300, .y = 300}));
    events.push_back(
        releaseAt(harness.codec, 2, {.x = 300, .y = 300}));
    events.push_back(pressAt(harness.codec, 3, tool));
    events.push_back(releaseAt(harness.codec, 3, tool));
    events.push_back(pressAt(harness.codec, 4, {.x = 295, .y = 295}));
    events.push_back(
        releaseAt(harness.codec, 4, {.x = 305, .y = 305}));
    events.push_back(chordAt(harness.codec, 5, Key::C));

    events.push_back(
        moveTo(harness.codec, 5, {.x = 500, .y = 300}));
    events.push_back(keyAt(harness.codec, 6, Key::Grave));
    events.push_back(
        chordAt(harness.codec, 6 + kConsoleAnimTicks, Key::V));

    events.push_back(
        keyAt(harness.codec, 6 + kConsoleAnimTicks, Key::Grave));
    const Tick closed = 6 + 2 * kConsoleAnimTicks + 1;
    events.push_back(
        moveTo(harness.codec, closed, {.x = 600, .y = 300}));
    events.push_back(chordAt(harness.codec, closed, Key::V));
    events.push_back(stopAt(closed + 1));
    ReplaySource source(std::move(events));

    const auto summary = harness.run(source, 60, "unused.json");

    EXPECT_EQ(summary.edits, 2U);
    EXPECT_TRUE(summary.console.empty());
}

TEST(ConsoleSinkTest, DumpState_WritesTheDocumentAndItsPngs)
{
    const ScratchDirectory dir("antwika_atlas_console_dump.");
    const auto path = (dir.path() / "dump_state.json").string();

    ConsoleHarness harness;
    std::vector<TickEvent> events;
    events.push_back(pressAt(harness.codec, 1, {.x = 300, .y = 300}));
    events.push_back(
        releaseAt(harness.codec, 1, {.x = 300, .y = 300}));
    events.push_back(keyAt(harness.codec, 2, Key::Grave));
    typeText(
        events, harness.codec, 2 + kConsoleAnimTicks, "dump_state");
    events.push_back(
        keyAt(harness.codec, 2 + kConsoleAnimTicks, Key::Enter));
    events.push_back(stopAt(3 + kConsoleAnimTicks));
    ReplaySource source(std::move(events));

    const auto summary = harness.run(source, 40, path);

    EXPECT_TRUE(std::filesystem::exists(path));
    EXPECT_TRUE(std::filesystem::exists(
        dir.path() / "dump_state.sheet.png"));

    const auto dumped = readDump(path);
    const auto state =
        antwika::atlas_editor::stateDumpFromJson(dumped.state);

    EXPECT_EQ(state.sheet.size, kCanvas);
    EXPECT_EQ(state.changes, 1U);
    EXPECT_EQ(state.tool, Tool::Paint);
    EXPECT_EQ(
        dumped.console,
        (std::vector<std::string>{
            "> dump_state", "dumped state to " + path}));
    EXPECT_EQ(summary.console, dumped.console);
}

TEST(ConsoleSinkTest, LoadState_ComesBackToTheDumpedInstant)
{
    const ScratchDirectory dir("antwika_atlas_console_load.");
    const auto path = (dir.path() / "dump_state.json").string();

    {
        ConsoleHarness harness;
        std::vector<TickEvent> events;
        events.push_back(
            pressAt(harness.codec, 1, {.x = 300, .y = 300}));
        events.push_back(
            releaseAt(harness.codec, 1, {.x = 300, .y = 300}));
        events.push_back(keyAt(harness.codec, 2, Key::Grave));
        typeText(
            events, harness.codec, 2 + kConsoleAnimTicks,
            "dump_state");
        events.push_back(
            keyAt(harness.codec, 2 + kConsoleAnimTicks, Key::Enter));
        events.push_back(stopAt(3 + kConsoleAnimTicks));
        ReplaySource source(std::move(events));

        harness.run(source, 40, path);
    }

    ConsoleHarness fresh;
    std::vector<TickEvent> events{keyAt(fresh.codec, 1, Key::Grave)};
    typeText(events, fresh.codec, kOpenTick, "load_state");
    events.push_back(keyAt(fresh.codec, kOpenTick, Key::Enter));
    events.push_back(keyAt(fresh.codec, kOpenTick, Key::Grave));

    const Tick closed = kOpenTick + kConsoleAnimTicks + 1;
    events.push_back(
        pressAt(fresh.codec, closed, {.x = 300, .y = 300}));
    events.push_back(
        releaseAt(fresh.codec, closed, {.x = 300, .y = 300}));
    events.push_back(stopAt(closed + 1));
    ReplaySource source(std::move(events));

    const auto summary = fresh.run(source, 60, path);

    EXPECT_EQ(summary.edits, 1U);
}

TEST(ConsoleSinkTest, Run_MountsNoConsoleWithoutAnOverlay)
{
    NiceMock<MockLogger> logger;
    NiceMock<MockEventSink> eventSink;
    const InputEventCodec codec;
    FakeMemoryStore store;

    std::vector<TickEvent> events{
        keyAt(codec, 1, Key::Grave), stopAt(3)};
    ReplaySource source(std::move(events));

    const auto summary = bootstrap(EditorWiring{
        .logger = logger,
        .eventSink = eventSink,
        .inputSource = source,
        .codec = codec,
        .store = store,
        .translator = kTranslator,
        .canvas = kCanvas,
        .blank = kCanvas,
        .tiles = TileGrid{},
        .maxTicks = 10});

    EXPECT_TRUE(summary.console.empty());
}
