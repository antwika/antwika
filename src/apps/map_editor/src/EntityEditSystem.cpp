#include "antwika/map_editor/EntityEditSystem.hpp"

#include <cstddef>
#include <string>
#include <variant>

#include <antwika/tilemap/Entities.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/map_editor/Commands.hpp"
#include "antwika/map_editor/Components.hpp"
#include "antwika/map_editor/Widgets.hpp"

namespace antwika::map_editor
{

    EntityEditSystem::EntityEditSystem(EditorStore &store)
        : store(store)
    {
    }

    void EntityEditSystem::update(World &, antwika::time::Tick)
    {
        const auto acted = store.ui.acted;

        store.ui.acted = {};

        if (acted.activated == widgets::kPlace)
        {
            placeEntityKind(
                store.state,
                static_cast<MarkerKind>(
                    store.ui.placeKind % kMarkerKindCount));
            return;
        }

        if (acted.activated == widgets::kDelete)
        {
            removeEntitiesAtHovered(store.state);
            return;
        }

        if (acted.edit.has_value())
        {
            edit(*acted.edit);
        }
    }

    void EntityEditSystem::edit(const ui::TextEdit &change)
    {
        FieldBuffer *buffer = nullptr;

        if (change.field == widgets::kFieldId)
        {
            buffer = &store.ui.idField;
        }
        else if (change.field == widgets::kFieldTargetMap)
        {
            buffer = &store.ui.targetMapField;
        }
        else if (change.field == widgets::kFieldTargetEntry)
        {
            buffer = &store.ui.targetEntryField;
        }
        else if (change.field == widgets::kFieldTags)
        {
            buffer = &store.ui.tagsField;
        }
        else if (change.field == widgets::kCharName)
        {
            store.characters.nameField.text = change.text;
            store.characters.nameField.cursor = change.cursor;
            return;
        }

        if (buffer == nullptr)
        {
            return;
        }

        if (change.cancelled)
        {
            loadEntityBuffers(store);
            store.ui.focus = ui::kNoWidget;
            return;
        }

        buffer->text = change.text;
        buffer->cursor = change.cursor;

        if (change.submitted)
        {
            applyBuffers();
        }
    }

    void EntityEditSystem::applyBuffers()
    {
        if (!store.ui.selected.has_value())
        {
            return;
        }

        const auto index = *store.ui.selected;
        const auto &entities = store.state.map.entities();

        if (index >= entities.size())
        {
            return;
        }

        auto edited = entities[index];

        std::visit(
            [this](auto &kind) { kind.id = store.ui.idField.text; },
            edited);

        if (auto *transition = std::get_if<tilemap::Transition>(&edited))
        {
            transition->targetMap = store.ui.targetMapField.text;
            transition->targetEntry = store.ui.targetEntryField.text;
        }

        if (auto *pickup = std::get_if<tilemap::Pickup>(&edited))
        {
            pickup->grantedTags = splitTags(store.ui.tagsField.text);
        }

        replaceEntity(store.state, index, std::move(edited));
    }

}
