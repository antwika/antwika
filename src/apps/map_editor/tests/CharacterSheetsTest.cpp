#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <vector>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/PngReader.hpp>
#include <antwika/gfx/PngWriter.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/SizeF.hpp>
#include <antwika/gfx/ViewportRenderer.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockTexture.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/testing/ScratchPath.hpp>
#include <antwika/tilemap/MapHeader.hpp>
#include <antwika/tilemap/TileMap.hpp>

#include "antwika/map_editor/CharacterSheets.hpp"

using antwika::gfx::Bitmap;
using antwika::gfx::Color;
using antwika::gfx::ITexture;
using antwika::gfx::Point;
using antwika::gfx::PointF;
using antwika::gfx::RectF;
using antwika::gfx::Size;
using antwika::gfx::SizeF;
using antwika::gfx::ViewportRenderer;
using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::mocks::MockTexture;
using antwika::log::Level;
using antwika::log::mocks::MockLogger;
using antwika::map_editor::characterFrameSource;
using antwika::map_editor::CharacterDoc;
using antwika::map_editor::characterPixelAt;
using antwika::map_editor::deleteCharacterFiles;
using antwika::map_editor::drawCharacterWorkspace;
using antwika::map_editor::EditorStore;
using antwika::map_editor::kCharacterLeft;
using antwika::map_editor::kCharacterSize;
using antwika::map_editor::kCharacterTop;
using antwika::map_editor::kCharacterZoom;
using antwika::map_editor::loadCharacters;
using antwika::map_editor::placeholderCharacter;
using antwika::map_editor::rowNameOf;
using antwika::map_editor::saveCharacter;
using antwika::map_editor::saveSelectedCharacter;
using antwika::testing::ScratchDirectory;
using antwika::tilemap::MapHeader;
using antwika::tilemap::TileMap;
using ::testing::_;
using ::testing::AnyNumber;
using ::testing::Field;
using ::testing::HasSubstr;
using ::testing::NiceMock;
using ::testing::Ref;
using ::testing::SaveArg;

namespace
{
    constexpr std::int32_t kExtent =
        static_cast<std::int32_t>(kCharacterSize) * kCharacterZoom;

    constexpr Size kCanvas{.width = 320, .height = 280};

    [[nodiscard]] Bitmap sheetOf(
        const std::uint32_t width, const std::uint32_t height)
    {
        return Bitmap{
            .size = {.width = width, .height = height},
            .pixels = std::vector<std::uint8_t>(
                static_cast<std::size_t>(width) * height
                    * antwika::gfx::kBytesPerPixel,
                0)};
    }

    void writeSheet(
        const std::filesystem::path &where, const Bitmap &sheet)
    {
        std::ofstream out(where, std::ios::binary);
        antwika::gfx::PngWriter{}.write(sheet, out);
    }

    [[nodiscard]] Bitmap readSheet(const std::filesystem::path &where)
    {
        std::ifstream in(where, std::ios::binary);

        return antwika::gfx::PngReader{}.read(in);
    }

    [[nodiscard]] std::string readText(
        const std::filesystem::path &where)
    {
        std::ifstream in(where);

        return std::string{
            std::istreambuf_iterator<char>(in),
            std::istreambuf_iterator<char>()};
    }

    [[nodiscard]] std::uint8_t alphaAt(
        const Bitmap &sheet, const std::int32_t x, const std::int32_t y)
    {
        const auto offset =
            (static_cast<std::size_t>(y) * kCharacterSize
             + static_cast<std::size_t>(x))
            * antwika::gfx::kBytesPerPixel;

        return sheet.pixels[offset + 3];
    }

    void putPixel(
        Bitmap &sheet,
        const std::size_t index,
        const std::uint8_t gray,
        const std::uint8_t alpha)
    {
        const auto offset = index * antwika::gfx::kBytesPerPixel;
        sheet.pixels[offset] = gray;
        sheet.pixels[offset + 1] = gray;
        sheet.pixels[offset + 2] = gray;
        sheet.pixels[offset + 3] = alpha;
    }

    [[nodiscard]] EditorStore storeWith(
        const std::filesystem::path &directory)
    {
        EditorStore store{
            .state = {.map = TileMap{MapHeader{}, 2, 2}}};
        store.characters.directory = directory;

        return store;
    }

    [[nodiscard]] EditorStore storeWithHero(
        const std::filesystem::path &directory)
    {
        auto store = storeWith(directory);
        store.characters.list.push_back(CharacterDoc{.name = "hero"});
        store.characters.list[0].sheet.image = placeholderCharacter();
        store.characters.list[0].sheet.dirty = true;

        return store;
    }
}

TEST(CharacterSheetsTest, RowNameOf_NamesEveryDirectionRow)
{
    EXPECT_EQ(rowNameOf(0), "walk_down");
    EXPECT_EQ(rowNameOf(1), "walk_up");
    EXPECT_EQ(rowNameOf(2), "walk_left");
    EXPECT_EQ(rowNameOf(3), "walk_right");
}

TEST(CharacterSheetsTest, RowNameOf_YieldsAQuestionMarkOffTheTable)
{
    EXPECT_EQ(rowNameOf(-1), "?");
    EXPECT_EQ(rowNameOf(4), "?");
}

TEST(CharacterSheetsTest, CharacterPixelAt_DividesTheCanvasPointByTheZoom)
{
    const auto pixel = characterPixelAt(
        Point{
            .x = kCharacterLeft + 3 * kCharacterZoom + 1,
            .y = kCharacterTop + 5 * kCharacterZoom + 2});

    ASSERT_TRUE(pixel.has_value());
    EXPECT_EQ(pixel->x, 3);
    EXPECT_EQ(pixel->y, 5);
}

TEST(CharacterSheetsTest, CharacterPixelAt_YieldsNothingOffTheWorkspace)
{
    EXPECT_FALSE(
        characterPixelAt(
            Point{.x = kCharacterLeft - 1, .y = kCharacterTop})
            .has_value());
    EXPECT_FALSE(
        characterPixelAt(
            Point{.x = kCharacterLeft, .y = kCharacterTop - 1})
            .has_value());
    EXPECT_FALSE(
        characterPixelAt(
            Point{.x = kCharacterLeft + kExtent, .y = kCharacterTop})
            .has_value());
    EXPECT_FALSE(
        characterPixelAt(
            Point{.x = kCharacterLeft, .y = kCharacterTop + kExtent})
            .has_value());
}

TEST(CharacterSheetsTest, PlaceholderCharacter_DrawsAFigureInEveryFrame)
{
    const auto sheet = placeholderCharacter();

    EXPECT_EQ(sheet.size.width, kCharacterSize);
    EXPECT_EQ(sheet.size.height, kCharacterSize);

    for (std::int32_t row = 0; row < 4; ++row)
    {
        for (std::int32_t frame = 0; frame < 4; ++frame)
        {
            EXPECT_EQ(
                alphaAt(sheet, frame * 16 + 8, row * 16 + 9), 255)
                << row << ' ' << frame;
        }
    }
}

TEST(CharacterSheetsTest, PlaceholderCharacter_SpreadsTheLegsOnOddFrames)
{
    const auto sheet = placeholderCharacter();

    EXPECT_EQ(alphaAt(sheet, 5, 12), 0);
    EXPECT_EQ(alphaAt(sheet, 6, 12), 255);
    EXPECT_EQ(alphaAt(sheet, 16 + 5, 12), 255);
    EXPECT_EQ(alphaAt(sheet, 16 + 11, 12), 255);
}

TEST(CharacterSheetsTest, PlaceholderCharacter_MarksTheFaceOfEachRow)
{
    const auto sheet = placeholderCharacter();

    EXPECT_EQ(alphaAt(sheet, 7, 4), 0);
    EXPECT_EQ(alphaAt(sheet, 9, 4), 0);
    EXPECT_EQ(alphaAt(sheet, 8, 4), 255);
    EXPECT_EQ(alphaAt(sheet, 7, 16 + 4), 255);
    EXPECT_EQ(alphaAt(sheet, 9, 16 + 4), 255);
    EXPECT_EQ(alphaAt(sheet, 6, 32 + 4), 0);
    EXPECT_EQ(alphaAt(sheet, 10, 48 + 4), 0);
}

TEST(CharacterSheetsTest, LoadCharacters_YieldsNothingForAMissingDirectory)
{
    NiceMock<MockLogger> logger;
    const ScratchDirectory scratch("characters.");

    EXPECT_TRUE(loadCharacters(scratch.path() / "absent", logger).empty());
}

TEST(CharacterSheetsTest, LoadCharacters_SortsTheSheetsByName)
{
    NiceMock<MockLogger> logger;
    const ScratchDirectory scratch("characters.");
    writeSheet(
        scratch.path() / "beta.png",
        sheetOf(kCharacterSize, kCharacterSize));
    writeSheet(
        scratch.path() / "alpha.png",
        sheetOf(kCharacterSize, kCharacterSize));

    const auto list = loadCharacters(scratch.path(), logger);

    ASSERT_EQ(list.size(), 2U);
    EXPECT_EQ(list[0].name, "alpha");
    EXPECT_EQ(list[1].name, "beta");
}

TEST(CharacterSheetsTest, LoadCharacters_SkipsWhatIsNotAPngFile)
{
    NiceMock<MockLogger> logger;
    const ScratchDirectory scratch("characters.");
    scratch.write("notes.txt", "hello");
    scratch.write("hero.png_backup_from_yesterday", "hello");
    std::filesystem::create_directories(scratch.path() / "sub.png");

    EXPECT_TRUE(loadCharacters(scratch.path(), logger).empty());
}

TEST(CharacterSheetsTest, LoadCharacters_NamesASheetAfterItsFileStem)
{
    NiceMock<MockLogger> logger;
    const ScratchDirectory scratch("characters.");
    writeSheet(
        scratch.path() / "wandering_merchant_with_a_cart.png",
        sheetOf(kCharacterSize, kCharacterSize));

    const auto list = loadCharacters(scratch.path(), logger);

    ASSERT_EQ(list.size(), 1U);
    EXPECT_EQ(list[0].name, "wandering_merchant_with_a_cart");
}

TEST(CharacterSheetsTest, LoadCharacters_WarnsAboutASheetOfTheWrongSize)
{
    NiceMock<MockLogger> logger;
    const ScratchDirectory scratch("characters.");
    writeSheet(
        scratch.path() / "narrow.png", sheetOf(32, kCharacterSize));
    writeSheet(
        scratch.path() / "short.png", sheetOf(kCharacterSize, 32));

    EXPECT_CALL(
        logger, log(Level::Warning, HasSubstr("wrong character size")))
        .Times(2);

    EXPECT_TRUE(loadCharacters(scratch.path(), logger).empty());
}

TEST(CharacterSheetsTest, LoadCharacters_WarnsAboutAPngItCannotDecode)
{
    NiceMock<MockLogger> logger;
    const ScratchDirectory scratch("characters.");
    scratch.write("broken.png", "this is not a png");

    EXPECT_CALL(logger, log(Level::Warning, HasSubstr("decode"))).Times(1);

    EXPECT_TRUE(loadCharacters(scratch.path(), logger).empty());
}

TEST(CharacterSheetsTest, LoadCharacters_SnapsTheSheetToInkAndPaper)
{
    NiceMock<MockLogger> logger;
    const ScratchDirectory scratch("characters.");
    auto sheet = sheetOf(kCharacterSize, kCharacterSize);
    putPixel(sheet, 0, 200, 255);
    putPixel(sheet, 1, 60, 255);
    writeSheet(scratch.path() / "hero.png", sheet);

    const auto list = loadCharacters(scratch.path(), logger);

    ASSERT_EQ(list.size(), 1U);
    EXPECT_EQ(list[0].sheet.image.pixels[0], 255);
    EXPECT_EQ(list[0].sheet.image.pixels[4], 128);
}

TEST(CharacterSheetsTest, SaveCharacter_WritesThePngIntoANewDirectory)
{
    const ScratchDirectory scratch("characters.");
    const auto directory = scratch.path() / "made";
    CharacterDoc character{.name = "hero"};
    character.sheet.image = placeholderCharacter();

    const auto error = saveCharacter(character, directory);

    EXPECT_FALSE(error.has_value());
    const auto written = readSheet(directory / "hero.png");
    EXPECT_EQ(written.size.width, kCharacterSize);
    EXPECT_EQ(written.pixels, character.sheet.image.pixels);
}

TEST(CharacterSheetsTest, SaveCharacter_DescribesTheFrameTableInTheSidecar)
{
    const ScratchDirectory scratch("characters.");
    CharacterDoc character{.name = "hero"};
    character.sheet.image = placeholderCharacter();

    const auto error = saveCharacter(character, scratch.path());

    EXPECT_FALSE(error.has_value());
    const auto sidecar = readText(scratch.path() / "hero.json");
    EXPECT_THAT(sidecar, HasSubstr("\"size\": 64"));
    EXPECT_THAT(sidecar, HasSubstr("\"frame\": 16"));
    EXPECT_THAT(sidecar, HasSubstr("\"columns\": 4"));
    EXPECT_THAT(sidecar, HasSubstr("\"walk_right\""));
}

TEST(CharacterSheetsTest, SaveCharacter_ReportsAPngItCannotOpen)
{
    const ScratchDirectory scratch("characters.");
    std::filesystem::create_directories(scratch.path() / "hero.png");
    CharacterDoc character{.name = "hero"};
    character.sheet.image = placeholderCharacter();

    const auto error = saveCharacter(character, scratch.path());

    ASSERT_TRUE(error.has_value());
    EXPECT_THAT(*error, HasSubstr("cannot open"));
}

TEST(CharacterSheetsTest, SaveCharacter_ReportsAPngItCannotEncode)
{
    const ScratchDirectory scratch("characters.");
    CharacterDoc character{.name = "hero"};
    character.sheet.image.size = Size{.width = 8, .height = 8};

    const auto error = saveCharacter(character, scratch.path());

    ASSERT_TRUE(error.has_value());
    EXPECT_THAT(*error, HasSubstr("cannot write"));
    EXPECT_FALSE(std::filesystem::exists(scratch.path() / "hero.json"));
}

TEST(CharacterSheetsTest, SaveCharacter_ReportsASidecarItCannotOpen)
{
    const ScratchDirectory scratch("characters.");
    std::filesystem::create_directories(scratch.path() / "hero.json");
    CharacterDoc character{.name = "hero"};
    character.sheet.image = placeholderCharacter();

    const auto error = saveCharacter(character, scratch.path());

    ASSERT_TRUE(error.has_value());
    EXPECT_THAT(*error, HasSubstr("sidecar for hero"));
}

TEST(CharacterSheetsTest, DeleteCharacterFiles_RemovesThePngAndTheSidecar)
{
    const ScratchDirectory scratch("characters.");
    scratch.write("hero.png", "png");
    scratch.write("hero.json", "{}");
    scratch.write("other.png", "png");

    deleteCharacterFiles("hero", scratch.path());

    EXPECT_FALSE(std::filesystem::exists(scratch.path() / "hero.png"));
    EXPECT_FALSE(std::filesystem::exists(scratch.path() / "hero.json"));
    EXPECT_TRUE(std::filesystem::exists(scratch.path() / "other.png"));
}

TEST(CharacterSheetsTest, DeleteCharacterFiles_LeavesAbsentFilesAlone)
{
    const ScratchDirectory scratch("characters.");

    deleteCharacterFiles("hero", scratch.path());

    EXPECT_TRUE(std::filesystem::is_empty(scratch.path()));
}

TEST(CharacterSheetsTest, SaveSelectedCharacter_ReportsAnEmptyList)
{
    NiceMock<MockLogger> logger;
    const ScratchDirectory scratch("characters.");
    auto store = storeWith(scratch.path());

    saveSelectedCharacter(store, logger);

    EXPECT_EQ(store.characters.message, "nothing to save");
    EXPECT_TRUE(std::filesystem::is_empty(scratch.path()));
}

TEST(CharacterSheetsTest, SaveSelectedCharacter_ClearsTheDirtyFlag)
{
    NiceMock<MockLogger> logger;
    const ScratchDirectory scratch("characters.");
    auto store = storeWithHero(scratch.path());
    store.characters.message = "stale";

    EXPECT_CALL(logger, log(Level::Info, HasSubstr("saved character hero")))
        .Times(1);

    saveSelectedCharacter(store, logger);

    EXPECT_FALSE(store.characters.list[0].sheet.dirty);
    EXPECT_TRUE(store.characters.message.empty());
    EXPECT_TRUE(std::filesystem::exists(scratch.path() / "hero.png"));
}

TEST(CharacterSheetsTest, SaveSelectedCharacter_ShowsTheFailureInThePanel)
{
    NiceMock<MockLogger> logger;
    const ScratchDirectory scratch("characters.");
    scratch.write("blocked", "not a directory");
    auto store = storeWithHero(scratch.path() / "blocked" / "deeper");

    EXPECT_CALL(logger, log(Level::Error, HasSubstr("cannot open")))
        .Times(1);

    saveSelectedCharacter(store, logger);

    EXPECT_THAT(store.characters.message, HasSubstr("cannot open"));
    EXPECT_TRUE(store.characters.list[0].sheet.dirty);
}

TEST(CharacterSheetsTest, CharacterFrameSource_CutsOutTheFrameCell)
{
    EXPECT_EQ(
        characterFrameSource(2, 3),
        RectF({48.0F, 32.0F}, {16.0F, 16.0F}));
    EXPECT_EQ(
        characterFrameSource(0, 0),
        RectF({0.0F, 0.0F}, {16.0F, 16.0F}));
}

TEST(CharacterSheetsTest, DrawCharacterWorkspace_MagnifiesTheWholeSheet)
{
    NiceMock<MockRenderer> inner;
    NiceMock<MockTexture> texture;
    const ITexture &sheet = texture;
    ViewportRenderer view(inner, kCanvas, kCanvas);

    EXPECT_CALL(inner, drawTexture(_, _, _, _)).Times(AnyNumber());
    EXPECT_CALL(
        inner,
        drawTexture(
            Ref(sheet),
            RectF({0.0F, 0.0F}, {64.0F, 64.0F}),
            RectF({32.0F, 12.0F}, {256.0F, 256.0F}),
            _))
        .Times(1);

    drawCharacterWorkspace(view, sheet, std::nullopt, 0);
}

TEST(CharacterSheetsTest, DrawCharacterWorkspace_AlternatesTheBackdrop)
{
    NiceMock<MockRenderer> inner;
    NiceMock<MockTexture> texture;
    ViewportRenderer view(inner, kCanvas, kCanvas);

    Color first{};
    Color second{};
    Color third{};

    EXPECT_CALL(inner, drawRect(_, _)).Times(AnyNumber());
    EXPECT_CALL(
        inner, drawRect(RectF({32.0F, 12.0F}, {16.0F, 16.0F}), _))
        .WillOnce(SaveArg<1>(&first));
    EXPECT_CALL(
        inner, drawRect(RectF({48.0F, 12.0F}, {16.0F, 16.0F}), _))
        .WillOnce(SaveArg<1>(&second));
    EXPECT_CALL(
        inner, drawRect(RectF({64.0F, 12.0F}, {16.0F, 16.0F}), _))
        .WillOnce(SaveArg<1>(&third));

    drawCharacterWorkspace(view, texture, std::nullopt, 0);

    EXPECT_NE(first, second);
    EXPECT_EQ(first, third);
}

TEST(CharacterSheetsTest, DrawCharacterWorkspace_RulesThePixelGrid)
{
    NiceMock<MockRenderer> inner;
    NiceMock<MockTexture> texture;
    ViewportRenderer view(inner, kCanvas, kCanvas);

    EXPECT_CALL(inner, drawLine(_, _, _)).Times(AnyNumber());
    EXPECT_CALL(inner, drawLine(PointF{36.0F, 12.0F}, PointF{36.0F, 268.0F}, _))
        .Times(1);
    EXPECT_CALL(inner, drawLine(PointF{32.0F, 16.0F}, PointF{288.0F, 16.0F}, _))
        .Times(1);

    drawCharacterWorkspace(view, texture, std::nullopt, 0);
}

TEST(CharacterSheetsTest, DrawCharacterWorkspace_RulesAGuideOnEachFrame)
{
    NiceMock<MockRenderer> inner;
    NiceMock<MockTexture> texture;
    ViewportRenderer view(inner, kCanvas, kCanvas);

    Color grid{};
    Color guide{};

    EXPECT_CALL(inner, drawLine(_, _, _)).Times(AnyNumber());
    EXPECT_CALL(inner, drawLine(PointF{96.0F, 12.0F}, PointF{96.0F, 268.0F}, _))
        .WillOnce(SaveArg<2>(&grid))
        .WillOnce(SaveArg<2>(&guide));
    EXPECT_CALL(inner, drawLine(PointF{32.0F, 76.0F}, PointF{288.0F, 76.0F}, _))
        .Times(2);

    drawCharacterWorkspace(view, texture, std::nullopt, 0);

    EXPECT_NE(grid, guide);
}

TEST(CharacterSheetsTest, DrawCharacterWorkspace_AnimatesThePreview)
{
    constexpr std::uint32_t kSecondFrameTick = 8;

    NiceMock<MockRenderer> inner;
    NiceMock<MockTexture> texture;
    const ITexture &sheet = texture;
    ViewportRenderer view(inner, kCanvas, kCanvas);

    EXPECT_CALL(inner, drawTexture(_, _, _, _)).Times(AnyNumber());
    EXPECT_CALL(
        inner,
        drawTexture(
            Ref(sheet),
            RectF({16.0F, 0.0F}, {16.0F, 16.0F}),
            RectF({8.0F, 28.0F}, {16.0F, 16.0F}),
            _))
        .Times(1);

    drawCharacterWorkspace(view, sheet, std::nullopt, kSecondFrameTick);
}

TEST(CharacterSheetsTest, DrawCharacterWorkspace_PreviewsTheHoveredRow)
{
    NiceMock<MockRenderer> inner;
    NiceMock<MockTexture> texture;
    ViewportRenderer view(inner, kCanvas, kCanvas);

    EXPECT_CALL(inner, drawTexture(_, _, _, _)).Times(AnyNumber());
    EXPECT_CALL(
        inner,
        drawTexture(
            _,
            RectF({0.0F, 32.0F}, {16.0F, 16.0F}),
            RectF({8.0F, 28.0F}, {16.0F, 16.0F}),
            _))
        .Times(1);

    drawCharacterWorkspace(view, texture, Point{.x = 0, .y = 35}, 0);
}

TEST(CharacterSheetsTest, DrawCharacterWorkspace_TintsTheHoveredPixel)
{
    NiceMock<MockRenderer> inner;
    NiceMock<MockTexture> texture;
    ViewportRenderer view(inner, kCanvas, kCanvas);

    EXPECT_CALL(inner, drawRect(_, _)).Times(AnyNumber());
    EXPECT_CALL(inner, drawRect(RectF({44.0F, 32.0F}, {4.0F, 4.0F}), _))
        .Times(1);

    drawCharacterWorkspace(view, texture, Point{.x = 3, .y = 5}, 0);
}

TEST(CharacterSheetsTest, DrawCharacterWorkspace_OutlinesTheHoveredPixel)
{
    NiceMock<MockRenderer> inner;
    NiceMock<MockTexture> texture;
    ViewportRenderer view(inner, kCanvas, kCanvas);

    EXPECT_CALL(inner, drawRect(_, _)).Times(AnyNumber());
    EXPECT_CALL(inner, drawRect(RectF({44.0F, 32.0F}, {4.0F, 1.0F}), _))
        .Times(1);
    EXPECT_CALL(inner, drawRect(RectF({44.0F, 35.0F}, {4.0F, 1.0F}), _))
        .Times(1);

    drawCharacterWorkspace(view, texture, Point{.x = 3, .y = 5}, 0);
}

TEST(CharacterSheetsTest, DrawCharacterWorkspace_TintsNothingWithoutAHover)
{
    NiceMock<MockRenderer> inner;
    NiceMock<MockTexture> texture;
    ViewportRenderer view(inner, kCanvas, kCanvas);

    EXPECT_CALL(inner, drawRect(_, _)).Times(AnyNumber());
    EXPECT_CALL(
        inner, drawRect(Field(&RectF::size, SizeF{4.0F, 4.0F}), _))
        .Times(0);

    drawCharacterWorkspace(view, texture, std::nullopt, 0);
}
