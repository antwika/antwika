#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/pattern/Span.hpp>

namespace antwika::music_editor
{

    /**
     * @brief One occurrence of a section in the form.
     *
     * The form line is a list of these, in the order they play, and
     * the same name may occur as many times as the song asks for.
     */
    struct FormUse
    {
        /** @brief Which section plays here. */
        std::string name;

        /**
         * @brief How many cycles this occurrence plays for.
         *
         * Zero until resolveBars() runs, for an occurrence that
         * named no length of its own and takes the bars: default.
         */
        std::int64_t bars = 0;

        /**
         * @brief Compare two occurrences.
         * @param other The occurrence to compare against.
         * @return True when both fields match.
         */
        [[nodiscard]] bool operator==(const FormUse &other) const
            = default;
    };

    /**
     * @brief Read what follows a form: header.
     *
     * A token is a section name, optionally with a length of its
     * own: `verse` or `bridge/4`.
     *
     * @param text The line's text after the header, comment cut.
     * @return The occurrences, in the order they play.
     * @throws ScoreError If the line names nothing, a name holds a
     * character outside letters, digits and underscore, or a length
     * is not a whole number of bars between 1 and 1024.
     */
    [[nodiscard]] std::vector<FormUse> readFormLine(
        std::string_view text);

    /**
     * @brief Read what follows a bars: header.
     * @param text The line's text after the header, comment cut.
     * @return The default section length, in cycles.
     * @throws ScoreError If the line is not one whole number between
     * 1 and 1024.
     */
    [[nodiscard]] std::int64_t readBarsLine(std::string_view text);

    /**
     * @brief Read what follows a part: header.
     * @param text The line's text after the header, comment cut.
     * @return The section names the block's voices play in, first
     * occurrence order, duplicates dropped.
     * @throws ScoreError If the line names nothing or a name holds a
     * character outside letters, digits and underscore.
     */
    [[nodiscard]] std::vector<std::string> readPartLine(
        std::string_view text);

    /**
     * @brief Give every unmarked occurrence the default length.
     * @param uses The form, edited in place.
     * @param fallback The bars: default, or zero when there is none.
     * @throws ScoreError If an occurrence is unmarked and there is
     * no default to give it.
     */
    void resolveBars(std::vector<FormUse> &uses, std::int64_t fallback);

    /**
     * @brief Get how many cycles the whole form spans.
     * @param uses The form, with every length resolved.
     * @return The sum of every occurrence's bars.
     */
    [[nodiscard]] std::int64_t periodOf(
        const std::vector<FormUse> &uses) noexcept;

    /**
     * @brief Get where a block's voices play inside one period.
     * @param parts The section names a part: header gave the block.
     * @param uses The form, with every length resolved.
     * @return A window per occurrence of any named section, in form
     * order; empty when the form plays none of them.
     */
    [[nodiscard]] std::vector<pattern::Span> windowsFor(
        const std::vector<std::string> &parts,
        const std::vector<FormUse> &uses);

} // namespace antwika::music_editor
