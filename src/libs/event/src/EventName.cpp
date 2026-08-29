#include "antwika/event/EventName.hpp"

#include <deque>
#include <mutex>
#include <ostream>
#include <unordered_map>

namespace antwika::event
{

    namespace
    {
        struct EventNameTable final
        {
            std::deque<std::string> texts;
            std::unordered_map<std::string_view, std::uint32_t> rowIds;
        };

        [[nodiscard]] EventNameTable getSeededTable()
        {
            EventNameTable table;

            for (const auto text : kSeededEventNames)
            {
                const auto id = static_cast<std::uint32_t>(table.texts.size());
                table.texts.emplace_back(text);
                table.rowIds.emplace(table.texts.back(), id);
            }

            return table;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] EventNameTable &getTable()
        {
            static EventNameTable table = getSeededTable();
            return table;
        }

        [[nodiscard]] std::mutex &getTableMutex()
        {
            static std::mutex mutex;
            return mutex;
        }

        [[nodiscard]] std::uint32_t getInternedId(std::string_view text)
        {
            const std::lock_guard<std::mutex> hold(getTableMutex());
            auto &table = getTable();

            const auto entry = table.rowIds.find(text);

            if (entry != table.rowIds.end())
            {
                return entry->second;
            }

            const auto id = static_cast<std::uint32_t>(table.texts.size());
            table.texts.emplace_back(text);
            table.rowIds.emplace(table.texts.back(), id);

            return id;
        }
    }

    EventName::EventName(std::string_view text) : id(getInternedId(text)) {}

    EventName::EventName(const char *text)
        : EventName(std::string_view(text))
    {
    }

    EventName::EventName(const std::string &text)
        : EventName(std::string_view(text))
    {
    }

    std::string_view EventName::getText() const
    {
        const std::lock_guard<std::mutex> hold(getTableMutex());
        return getTable().texts.at(id);
    }

    std::ostream &operator<<(std::ostream &out, const EventName &name)
    {
        return out << name.getText();
    }

}
