#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace antwika::i18n
{

    /**
     * @brief A symbolic name for one translatable string.
     *
     * A catalogue is keyed by this rather than by the English text, so a
     * locale that is missing a string produces a lookup that *reports* a
     * miss.
     * Keying by the English string would make a missing translation fall
     * through to English prose embedded in the calling code, which is
     * indistinguishable from a translation that happens to read the same
     * in both languages.
     */
    enum class MessageId : std::uint16_t
    {
        /**
         * @brief Main menu: start a new session.
         */
        MenuPlayGame,

        /**
         * @brief Main menu: open a recorded session.
         */
        MenuLoadReplay,

        /**
         * @brief Main menu: write the current session out.
         */
        MenuSaveReplay,

        /**
         * @brief Main menu: go back to the session already running.
         */
        MenuResumeGame,

        /**
         * @brief Main menu: the label above the language choices.
         */
        MenuLanguage,

        /**
         * @brief The name of the English language.
         */
        LanguageEnglish,

        /**
         * @brief The name of the Swedish language.
         */
        LanguageSwedish,

        /**
         * @brief Toolbar: zoom one level in.
         */
        ToolbarZoomIn,

        /**
         * @brief Toolbar: zoom one level out.
         */
        ToolbarZoomOut,

        /**
         * @brief Toolbar: put the camera back where the run started.
         */
        ToolbarResetView,

        /**
         * @brief Toolbar: the current zoom level, with `{0}` as the
         *        level.
         */
        ToolbarZoomLevel,
    };

    /**
     * @brief How many ids a complete catalogue carries.
     */
    inline constexpr std::size_t kMessageCount{11};

    /**
     * @brief Every id, in declaration order.
     *
     * This is what makes the symbolic key pay for itself: a test can walk
     * the whole set and assert that every locale answers for all of it,
     * which is not expressible when the key *is* the English text.
     */
    inline constexpr std::array<MessageId, kMessageCount> kAllMessageIds{
        MessageId::MenuPlayGame,
        MessageId::MenuLoadReplay,
        MessageId::MenuSaveReplay,
        MessageId::MenuResumeGame,
        MessageId::MenuLanguage,
        MessageId::LanguageEnglish,
        MessageId::LanguageSwedish,
        MessageId::ToolbarZoomIn,
        MessageId::ToolbarZoomOut,
        MessageId::ToolbarResetView,
        MessageId::ToolbarZoomLevel,
    };

    /**
     * @brief The id's own name, for diagnostics and for the text a total
     *        lookup falls back on.
     * @param id The id to name.
     * @return The enumerator's name, or `"?"` for a value that is not one
     *         of the enumerators.
     */
    [[nodiscard]] std::string_view nameOf(MessageId id) noexcept;

} // namespace antwika::i18n
