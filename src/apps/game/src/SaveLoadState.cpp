#include "antwika/game/SaveLoadState.hpp"

#include <algorithm>
#include <utility>

namespace antwika::game
{

    SaveLoadState::SaveLoadState(std::vector<std::string> saves)
        : names(std::move(saves))
    {
        std::sort(names.begin(), names.end());
        reindex();

        if (!names.empty())
        {
            chosen = 0;
        }
    }

    std::span<const std::string_view> SaveLoadState::options() const noexcept
    {
        return views;
    }

    std::size_t SaveLoadState::selected() const noexcept
    {
        return chosen;
    }

    std::string_view SaveLoadState::selectedName() const noexcept
    {
        if (chosen >= names.size())
        {
            return {};
        }

        return names[chosen];
    }

    void SaveLoadState::select(std::size_t index) noexcept
    {
        chosen = index < names.size() ? index : antwika::ui::kNoOption;
    }

    void SaveLoadState::add(const std::string &name)
    {
        const auto at = std::lower_bound(names.begin(), names.end(), name);

        if (at == names.end() || *at != name)
        {
            names.insert(at, name);
            reindex();
        }

        chosen = static_cast<std::size_t>(
            std::lower_bound(names.begin(), names.end(), name)
            - names.begin());
    }

    bool SaveLoadState::listOpen() const noexcept
    {
        return open;
    }

    void SaveLoadState::setListOpen(bool showing) noexcept
    {
        open = showing;
    }

    const std::string &SaveLoadState::name() const noexcept
    {
        return typed;
    }

    std::size_t SaveLoadState::caret() const noexcept
    {
        return cursor;
    }

    void SaveLoadState::setName(std::string text, std::size_t caretAt)
    {
        typed = std::move(text);
        cursor = caretAt;
    }

    WidgetId SaveLoadState::focus() const noexcept
    {
        return focused;
    }

    void SaveLoadState::setFocus(WidgetId id) noexcept
    {
        focused = id;
    }

    const std::string &SaveLoadState::message() const noexcept
    {
        return note;
    }

    void SaveLoadState::setMessage(std::string text)
    {
        note = std::move(text);
    }

    void SaveLoadState::reindex()
    {
        // Rebuilt whole rather than appended to.
        // Inserting a name may have moved every one of their buffers.
        views.assign(names.begin(), names.end());
    }

} // namespace antwika::game
