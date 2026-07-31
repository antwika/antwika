#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/GfxError.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/atlas_editor/AtlasEditorError.hpp"
#include "antwika/atlas_editor/PngAtlasStore.hpp"

using antwika::atlas_editor::AtlasEditorError;
using antwika::atlas_editor::PngAtlasStore;
using antwika::gfx::Bitmap;
using antwika::gfx::GfxError;
using antwika::gfx::Size;

namespace
{
    // A file that removes itself.
    // A failing test then leaves nothing for the next run to find.
    class TempFile final
    {
    public:
        explicit TempFile(const std::string &name)
            : where(std::filesystem::temp_directory_path() / name)
        {
        }

        TempFile(const TempFile &) = delete;
        TempFile(TempFile &&) = delete;

        TempFile &operator=(const TempFile &) = delete;
        TempFile &operator=(TempFile &&) = delete;

        ~TempFile()
        {
            // The non-throwing overload.
            // A destructor that throws calls std::terminate.
            std::error_code code;
            std::filesystem::remove(where, code);
        }

        [[nodiscard]] std::string path() const
        {
            return where.string();
        }

    private:
        std::filesystem::path where;
    };

    Bitmap twoByTwo()
    {
        return Bitmap{
            .size = {.width = 2, .height = 2},
            .pixels = std::vector<std::uint8_t>{
                255, 0,   0,   255, 0,  255, 0,   255,
                0,   0,   255, 0,   17, 34,  51,  128}};
    }
} // namespace

TEST(PngAtlasStoreTest, SaveThenLoad_GivesBackEveryByte)
{
    const TempFile file("antwika_atlas_editor_roundtrip.png");
    PngAtlasStore store(file.path(), file.path());

    store.save(twoByTwo());

    const auto loaded = store.load();

    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(*loaded, twoByTwo());
}

TEST(PngAtlasStoreTest, Load_AnswersNothingWhenThereIsNowhereToLoadFrom)
{
    PngAtlasStore store(std::nullopt, std::nullopt);

    EXPECT_FALSE(store.load().has_value());
}

TEST(PngAtlasStoreTest, Load_ThrowsWhenTheFileIsNotThere)
{
    PngAtlasStore store(
        std::string("/nonexistent/antwika/nothing.png"), std::nullopt);

    EXPECT_THROW(static_cast<void>(store.load()), GfxError);
}

TEST(PngAtlasStoreTest, Load_ThrowsWhenTheFileIsNotAPng)
{
    const TempFile file("antwika_atlas_editor_notapng.png");

    {
        std::ofstream out(file.path(), std::ios::binary);
        out << "this is not a png at all";
    }

    PngAtlasStore store(file.path(), std::nullopt);

    EXPECT_THROW(static_cast<void>(store.load()), GfxError);
}

TEST(PngAtlasStoreTest, Save_RefusesWhenThereIsNowhereToSaveTo)
{
    PngAtlasStore store(std::nullopt, std::nullopt);

    EXPECT_THROW(store.save(twoByTwo()), AtlasEditorError);
}

TEST(PngAtlasStoreTest, Save_ThrowsWhenTheFileCannotBeOpened)
{
    PngAtlasStore store(
        std::nullopt, std::string("/nonexistent/antwika/out.png"));

    EXPECT_THROW(store.save(twoByTwo()), GfxError);
}

TEST(PngAtlasStoreTest, SavePath_SaysWhereASaveWouldGo)
{
    const PngAtlasStore somewhere(
        std::nullopt, std::string("/tmp/out.png"));
    const PngAtlasStore nowhere(std::nullopt, std::nullopt);

    EXPECT_EQ(somewhere.savePath(), std::string("/tmp/out.png"));
    EXPECT_TRUE(nowhere.savePath().empty());
}

// The two paths are deliberately separate.
// Opening the game's atlas cannot overwrite it without an --out.
TEST(PngAtlasStoreTest, Save_WritesToItsOwnPathRatherThanTheOpenedOne)
{
    const TempFile opened("antwika_atlas_editor_in.png");
    const TempFile written("antwika_atlas_editor_out.png");

    PngAtlasStore seeding(std::nullopt, opened.path());
    seeding.save(twoByTwo());

    PngAtlasStore store(opened.path(), written.path());
    store.save(twoByTwo());

    EXPECT_TRUE(std::filesystem::exists(written.path()));
    EXPECT_EQ(store.savePath(), written.path());
}
