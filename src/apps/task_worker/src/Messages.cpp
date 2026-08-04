#include "antwika/task_worker/Messages.hpp"

#include <array>
#include <cstddef>
#include <span>

#include <antwika/i18n/Catalogue.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/i18n/MessageName.hpp>

#include "antwika/task_worker/MessageId.hpp"

namespace antwika::task_worker
{

    namespace
    {

        using i18n::Catalogue;
        using i18n::CatalogueEntry;
        using i18n::Locale;
        using i18n::MessageName;

        constexpr std::array<MessageName<MessageId>, 11> kNames{{
            {MessageId::Tick, "Tick"},
            {MessageId::Budget, "Budget"},
            {MessageId::Started, "Started"},
            {MessageId::Workers, "Workers"},
            {MessageId::WorkerIdle, "WorkerIdle"},
            {MessageId::WorkerBusy, "WorkerBusy"},
            {MessageId::TicksLeft, "TicksLeft"},
            {MessageId::Queue, "Queue"},
            {MessageId::Queued, "Queued"},
            {MessageId::Blocked, "Blocked"},
            {MessageId::Completed, "Completed"},
        }};

        static_assert(
            kNames.size() == static_cast<std::size_t>(MessageId::Count),
            "every MessageId must appear in kNames exactly once");

        // Both arrays list every id, in the same order.
        // MessagesTest asserts they cover exactly kNames.
        // That assertion is the point of keying by id.
        // A forgotten Swedish entry is a red build, not a wrong label.
        constexpr std::array<CatalogueEntry<MessageId>, kNames.size()>
            kEnglishEntries{{
                {MessageId::Tick, "tick {0}"},
                {MessageId::Budget, "budget {0}"},
                {MessageId::Started, "started {0}"},
                {MessageId::Workers, "workers"},
                {MessageId::WorkerIdle, "worker {0} idle"},
                {MessageId::WorkerBusy, "worker {0} {1}"},
                {MessageId::TicksLeft, "{0} of {1} ticks left"},
                {MessageId::Queue, "queue"},
                {MessageId::Queued, "{0} priority {1}"},
                {MessageId::Blocked, "{0} waits for {1}"},
                {MessageId::Completed, "completed"},
            }};

        constexpr std::array<CatalogueEntry<MessageId>, kNames.size()>
            kSwedishEntries{{
                {MessageId::Tick, "tick {0}"},
                {MessageId::Budget, "budget {0}"},
                {MessageId::Started, "startade {0}"},
                {MessageId::Workers, "arbetare"},
                {MessageId::WorkerIdle, "arbetare {0} ledig"},
                {MessageId::WorkerBusy, "arbetare {0} {1}"},
                {MessageId::TicksLeft, "{0} av {1} tick kvar"},
                {MessageId::Queue, "kö"},
                {MessageId::Queued, "{0} prioritet {1}"},
                {MessageId::Blocked, "{0} väntar på {1}"},
                {MessageId::Completed, "klara"},
            }};

        constexpr Catalogue<MessageId> kEnglishCatalogue{
            Locale::English, kEnglishEntries};

        constexpr Catalogue<MessageId> kSwedishCatalogue{
            Locale::Swedish, kSwedishEntries};

    } // namespace

    std::span<const i18n::MessageName<MessageId>>
        Messages::names() noexcept
    {
        return kNames;
    }

    const i18n::Catalogue<MessageId> &Messages::catalogueFor(
        i18n::Locale locale) noexcept
    {
        return antwika::i18n::pickCatalogue(
            locale, kEnglishCatalogue, kSwedishCatalogue);
    }

} // namespace antwika::task_worker
