#include "antwika/map_editor/EditorStore.hpp"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <antwika/tilemap/Entities.hpp>
#include <antwika/tileset/TilesetFile.hpp>

namespace antwika::map_editor
{

    namespace
    {
        constexpr float kMinVisible = 48.0F;

        void clearBuffers(UiSession &session)
        {
            session.idField = {};
            session.targetMapField = {};
            session.targetEntryField = {};
            session.tagsField = {};
        }

        [[nodiscard]] FieldBuffer bufferOf(std::string text)
        {
            const auto cursor = text.size();

            FieldBuffer buffer;
            buffer.text = std::move(text);
            buffer.cursor = cursor;

            return buffer;
        }

        [[nodiscard]] std::string idOf(const tilemap::Entity &entity)
        {
            return std::visit(
                [](const auto &kind) { return kind.id; }, entity);
        }
    }

    SheetDoc *activeSheet(EditorStore &store)
    {
        if (store.view == EditorView::Characters)
        {
            auto &characters = store.characters;

            if (characters.selected >= characters.list.size())
            {
                return nullptr;
            }

            return &characters.list[characters.selected].sheet;
        }

        return nullptr;
    }

    TilesetDoc *activeTilesetDoc(EditorStore &store)
    {
        auto &open = store.tilesets.open;

        if (open.empty())
        {
            return nullptr;
        }

        return &open[store.tilesets.active % open.size()];
    }

    const TilesetDoc *activeTilesetDoc(const EditorStore &store)
    {
        const auto &open = store.tilesets.open;

        if (open.empty())
        {
            return nullptr;
        }

        return &open[store.tilesets.active % open.size()];
    }

    gfx::Point mapPointOf(
        const gfx::Point canvas, const MapCamera &camera) noexcept
    {
        const auto zoom = camera.zoom();
        const auto localX =
            static_cast<float>(canvas.x) - camera.panX;
        const auto localY =
            static_cast<float>(canvas.y - kMenuBarHeight)
            - camera.panY;

        return gfx::Point{
            .x = static_cast<std::int32_t>(localX / zoom),
            .y = static_cast<std::int32_t>(localY / zoom)};
    }

    void togglePicker(EditorStore &store)
    {
        auto &picker = store.picker;

        picker.active = !picker.active;
        picker.pending.reset();
        picker.walkCell.reset();
        picker.walkDepth = 0;
        picker.hover.clear();
    }

    void cycleEditorView(EditorStore &store)
    {
        switch (store.view)
        {
            case EditorView::Map:
                store.view = EditorView::Tiles;
                return;
            case EditorView::Tiles:
                store.view = EditorView::Characters;
                return;
            default:
                store.view = EditorView::Map;
                return;
        }
    }

    void cycleEditorViewBack(EditorStore &store)
    {
        switch (store.view)
        {
            case EditorView::Map:
                store.view = EditorView::Characters;
                return;
            case EditorView::Characters:
                store.view = EditorView::Tiles;
                return;
            default:
                store.view = EditorView::Map;
                return;
        }
    }

    void clampCamera(
        MapCamera &camera,
        const float mapWidth,
        const float mapHeight)
    {
        const auto zoom = camera.zoom();

        const auto lowX = kMinVisible - mapWidth * zoom;
        const auto highX =
            static_cast<float>(kMapViewWidth) - kMinVisible;
        const auto lowY = kMinVisible - mapHeight * zoom;
        const auto highY =
            static_cast<float>(kMapViewHeight) - kMinVisible;

        camera.panX = std::clamp(camera.panX, lowX, highX);
        camera.panY = std::clamp(camera.panY, lowY, highY);
    }

    void zoomAt(
        MapCamera &camera,
        const float anchorX,
        const float anchorY,
        const std::int32_t direction,
        const float mapWidth,
        const float mapHeight)
    {
        const auto oldZoom = camera.zoom();

        if (direction > 0 && camera.step + 1 < kZoomStepCount)
        {
            ++camera.step;
        }
        else if (direction < 0 && camera.step > 0)
        {
            --camera.step;
        }
        else
        {
            return;
        }

        const auto newZoom = camera.zoom();

        camera.panX =
            anchorX - (anchorX - camera.panX) / oldZoom * newZoom;
        camera.panY =
            anchorY - (anchorY - camera.panY) / oldZoom * newZoom;

        clampCamera(camera, mapWidth, mapHeight);
    }

    namespace
    {
        [[nodiscard]] bool isJsonName(const std::string &name)
        {
            const std::string suffix = ".json";

            return name.size() > suffix.size()
                   && name.compare(
                          name.size() - suffix.size(),
                          suffix.size(),
                          suffix)
                          == 0;
        }
    }

    void openFileDialog(
        EditorStore &store,
        const DialogMode mode,
        const DialogTarget target)
    {
        auto &dialog = store.dialog;

        dialog.mode = mode;
        dialog.target = target;
        dialog.message.clear();

        if (target == DialogTarget::Tileset)
        {
            dialog.directory = store.tilesets.directory.string();

            const auto *doc = activeTilesetDoc(store);
            const auto name = doc != nullptr && mode == DialogMode::SaveAs
                                  ? doc->data.name
                                  : std::string{};

            FieldBuffer named;
            named.text = name;
            named.cursor = name.size();
            dialog.nameField = std::move(named);
            refreshDialogEntries(dialog);
            return;
        }

        const auto parent = store.state.path.parent_path();

        if (parent.empty())
        {
            dialog.directory = ".";
        }
        else
        {
            dialog.directory = parent.string();
        }

        const auto name = store.state.path.filename().string();

        FieldBuffer named;

        if (mode == DialogMode::SaveAs)
        {
            named.text = name;
            named.cursor = name.size();
        }

        dialog.nameField = std::move(named);

        refreshDialogEntries(dialog);
    }

    void refreshDialogEntries(FileDialog &dialog)
    {
        if (dialog.target == DialogTarget::Tileset)
        {
            dialog.entries.clear();

            for (auto &name : tileset::listTilesets(dialog.directory))
            {
                io::FileEntry entry;
                entry.name = std::move(name);
                entry.directory = false;
                dialog.entries.push_back(std::move(entry));
            }

            dialog.page = 0;
            return;
        }

        auto listed = io::entriesIn(dialog.directory);

        std::vector<io::FileEntry> kept;

        for (auto &entry : listed)
        {
            if (entry.directory || isJsonName(entry.name))
            {
                kept.push_back(std::move(entry));
            }
        }

        dialog.entries = std::move(kept);
        dialog.page = 0;
    }

    void loadEntityBuffers(EditorStore &store)
    {
        clearBuffers(store.ui);

        if (!store.ui.selected.has_value())
        {
            return;
        }

        const auto &entities = store.state.map.entities();

        if (*store.ui.selected >= entities.size())
        {
            return;
        }

        const auto &entity = entities[*store.ui.selected];

        store.ui.idField = bufferOf(idOf(entity));

        if (const auto *transition =
                std::get_if<tilemap::Transition>(&entity))
        {
            store.ui.targetMapField = bufferOf(transition->targetMap);
            store.ui.targetEntryField =
                bufferOf(transition->targetEntry);
        }

        if (const auto *pickup = std::get_if<tilemap::Pickup>(&entity))
        {
            store.ui.tagsField = bufferOf(joinTags(pickup->grantedTags));
        }
    }

    std::string joinTags(const std::vector<std::string> &tags)
    {
        std::string joined;

        for (const auto &tag : tags)
        {
            if (!joined.empty())
            {
                joined += ',';
            }

            joined += tag;
        }

        return joined;
    } // GCOVR_EXCL_LINE

    std::vector<std::string> splitTags(const std::string &joined)
    {
        std::vector<std::string> tags;
        std::string current;

        for (const char letter : joined)
        {
            if (letter == ',')
            {
                if (!current.empty())
                {
                    tags.push_back(current);
                    current.clear();
                }

                continue;
            }

            if (letter != ' ')
            {
                current.push_back(letter);
            }
        }

        if (!current.empty())
        {
            tags.push_back(current);
        }

        return tags;
    }

}
