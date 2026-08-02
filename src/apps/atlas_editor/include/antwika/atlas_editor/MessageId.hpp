#pragma once

#include <cstdint>

namespace antwika::atlas_editor
{

    /**
     * @brief Every string the atlas editor shows, as symbolic ids.
     *
     * They live here rather than in antwika::i18n because a library
     * that enumerated its consumers' strings would be a library naming
     * its consumers.
     * What keeps that safe is that the list of every id there is, both
     * catalogues and the completeness check over them are in this
     * module too: see the MessageSet concept in
     * <antwika/i18n/MessageSet.hpp> and the suite MessagesTest.cpp
     * instantiates.
     *
     * A MessageId is never persisted, so its numbering is free and
     * adding, reordering or removing one needs no migration.
     */
    enum class MessageId : std::uint16_t
    {
        /**
         * @brief The tool that puts a colour down.
         */
        ToolPaint,

        /**
         * @brief The tool that clears a pixel.
         */
        ToolErase,

        /**
         * @brief The tool that takes a colour.
         */
        ToolPick,

        /**
         * @brief Put the whole sheet back in the middle.
         */
        ResetView,

        /**
         * @brief Show or hide the slot grid.
         */
        Grid,

        /**
         * @brief Read a sheet back in.
         */
        Load,

        /**
         * @brief Write the sheet out.
         */
        Save,

        /**
         * @brief The pointer is on no pixel of the sheet.
         */
        PixelUnknown,

        /**
         * @brief Which pixel the pointer is on, `{0}` across and `{1}` down.
         */
        PixelAt,

        /**
         * @brief Which slot that pixel falls in, `{0}`.
         */
        Slot,

        /**
         * @brief The sheet has changed since it was written.
         */
        Unsaved,

        /**
         * @brief The sheet was written to `{0}`.
         */
        Saved,

        /**
         * @brief The sheet was not written, because `{0}`. The reason is a
         *        diagnostic and is never translated.
         */
        SaveFailed,

        /**
         * @brief No sheet was named for a load to read.
         */
        NothingToLoad,

        /**
         * @brief A sheet was read in.
         */
        Loaded,

        /**
         * @brief No sheet was read, because `{0}`. The reason is a diagnostic
         *        and is never translated.
         */
        LoadFailed,

        /**
         * @brief How many ids there are; not an id itself.
         *
         * Messages.cpp static_asserts its name table against this,
         * which is what makes an enumerator nobody listed a build
         * failure rather than a string that is silently in no
         * catalogue.
         */
        Count,
    };

} // namespace antwika::atlas_editor
