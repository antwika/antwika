#include "antwika/atlas_editor/SnapshotStore.hpp"

#include <fstream>
#include <optional>
#include <string>
#include <utility>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/GfxError.hpp>
#include <antwika/gfx/PngReader.hpp>
#include <antwika/gfx/PngWriter.hpp>
#include <antwika/io/File.hpp>

#include "antwika/atlas_editor/AtlasEditorError.hpp"
#include "antwika/atlas_editor/Canvas.hpp"
#include "antwika/atlas_editor/OpeningSheet.hpp"
#include "antwika/atlas_editor/StateDump.hpp"

namespace antwika::atlas_editor
{

    using antwika::gfx::Bitmap;
    using antwika::gfx::GfxError;

    namespace
    {
        // The PNG paths are derived, never asked for.
        // One name on the console names the whole set of files.
        [[nodiscard]] std::string sidePath(
            const std::string &path, const std::string &suffix)
        {
            constexpr std::string_view kJson = ".json";

            if (path.ends_with(kJson))
            {
                return path.substr(0, path.size() - kJson.size())
                       + suffix;
            }

            return path + suffix;
        }

        [[nodiscard]] std::string sheetPathFor(const std::string &path)
        {
            return sidePath(path, ".sheet.png");
        }

        [[nodiscard]] std::string clipboardPathFor(
            const std::string &path)
        {
            return sidePath(path, ".clipboard.png");
        }

        void writePng(const std::string &path, const Bitmap &image)
        {
            std::ofstream file = io::openToWriteAs<GfxError>(
                path, "an image", io::Content::Bytes);

            // The writer flushes and checks the stream itself.
            // A refusal by the filesystem arrives as a GfxError.
            antwika::gfx::PngWriter{}.write(image, file);
        }

        [[nodiscard]] Bitmap readPng(const std::string &path)
        {
            std::ifstream file = io::openToReadAs<GfxError>(
                path, "an image", io::Content::Bytes);

            return antwika::gfx::PngReader{}.read(file);
        }

        // Read, and then held to the fingerprint the document names.
        // The two files travel apart, and only the pair is a dump.
        // A mismatch is refused whole rather than repaired.
        [[nodiscard]] Bitmap readBoundPng(
            const std::string &path, const DumpedImage &expected)
        {
            Bitmap image = readPng(path);

            if (fingerprintOf(image) != expected.fingerprint)
            {
                throw AtlasEditorError(
                    "atlas_editor: " + path
                    + " does not answer the fingerprint the dump "
                      "names; a dump edited by hand is not repaired");
            }

            return image;
        }

        [[nodiscard]] EditorStateDump takeDump(
            const EditorState &state)
        {
            EditorStateDump dump;

            dump.sheet = DumpedImage{
                .size = state.image().size(),
                .fingerprint = fingerprintOf(state.image().bitmap())};
            dump.sheetRevision = state.image().revision();

            if (state.clipboardImage().has_value())
            {
                dump.clipboard = DumpedImage{
                    .size = state.clipboardImage()->size(),
                    .fingerprint = fingerprintOf(
                        state.clipboardImage()->bitmap())};
            }

            dump.view = state.view();
            dump.tool = state.tool();
            dump.paint = state.color();
            dump.swatch = state.colorIndex();
            dump.showGrid = state.gridVisible();
            dump.showGuides = state.guidesVisible();
            dump.under = state.hovered();
            dump.marked = state.selection();
            dump.gesture = state.currentGesture();
            dump.changes = state.edits();
            dump.stepped = state.ticks();
            dump.written = state.saves();
            dump.read = state.loads();
            dump.savedRevision = state.savedAtRevision();

            return dump;
        }
    } // namespace

    EditorSnapshotStore::EditorSnapshotStore(
        EditorState &state) noexcept
        : console::JsonSnapshotStore<std::runtime_error>(
              {.magic = kStateDumpMagic,
               .version = kStateDumpVersion},
              "antwika atlas editor state dump document",
              standardStateDumpMigrations),
          state(state)
    {
    }

    nlohmann::json EditorSnapshotStore::takeState(
        const std::string &path)
    {
        // The bitmaps go first.
        // A document then only ever names PNGs already written.
        // A GfxError from either leaves dump() as a SnapshotError.
        writePng(sheetPathFor(path), state.image().bitmap());

        if (state.clipboardImage().has_value())
        {
            writePng(
                clipboardPathFor(path),
                state.clipboardImage()->bitmap());
        }

        return stateDumpToJson(takeDump(state));
    }

    void EditorSnapshotStore::applyState(
        const std::string &path, const nlohmann::json &dumped)
    {
        const auto dump = stateDumpFromJson(dumped);

        Canvas sheet(
            readBoundPng(sheetPathFor(path), dump.sheet),
            dump.sheetRevision);

        std::optional<Canvas> clipboard;
        if (dump.clipboard.has_value())
        {
            clipboard = Canvas(readBoundPng(
                clipboardPathFor(path), *dump.clipboard));
        }

        // The excluded line is the restore temporary's unwind arms.
        // Only a failed allocation inside it could take one.
        // See docs/confirming-unreachable-branches.md.
        state.restore(SessionRestore{ // GCOVR_EXCL_LINE
            .sheet = std::move(sheet),
            .clipboard = std::move(clipboard),
            .view = dump.view,
            .tool = dump.tool,
            .paint = dump.paint,
            .swatch = dump.swatch,
            .showGrid = dump.showGrid,
            .showGuides = dump.showGuides,
            .under = dump.under,
            .marked = dump.marked,
            .gesture = dump.gesture,
            .changes = dump.changes,
            .stepped = dump.stepped,
            .written = dump.written,
            .read = dump.read,
            .savedRevision = dump.savedRevision});
    }

} // namespace antwika::atlas_editor
