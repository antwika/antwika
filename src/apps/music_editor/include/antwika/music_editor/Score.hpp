#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/pattern/Pattern.hpp>
#include <antwika/pattern/Patterns.hpp>

#include "antwika/music_editor/FormUse.hpp"
#include "antwika/music_editor/NoteWords.hpp"
#include "antwika/music_editor/TrackPreset.hpp"
#include "antwika/music_editor/VoiceChain.hpp"

namespace antwika::music_editor
{

    using antwika::pattern::Pattern;

    [[nodiscard]] std::string openingSource();

    struct Problem final
    {
        std::size_t line = 0;

        std::string message{};

        [[nodiscard]] bool operator==(const Problem &other) const
            = default;
    };

    struct Voice final
    {
        TrackPreset preset{};

        Pattern playing = pattern::silence();
    };

    struct Pianoroll final
    {
        Pattern playing = pattern::silence();

        TrackPreset preset{};

        std::size_t line = 0;
    };

    struct Waveform final
    {
        Pattern playing = pattern::silence();

        TrackPreset preset{};

        std::size_t line = 0;

        std::string_view chain{};
    };

    struct DocumentSpan final
    {
        std::size_t begin = 0;

        std::size_t end = 0;

        [[nodiscard]] bool operator==(const DocumentSpan &other) const
            = default;
    };

    class Score final
    {
    public:
        Score();

        Score(const Score &) = delete;
        Score(Score &&) = delete;

        Score &operator=(const Score &) = delete;
        Score &operator=(Score &&) = delete;

        void read(const std::string &source);

        [[nodiscard]] const std::vector<Voice> &voices() const noexcept;

        [[nodiscard]] const std::vector<Pianoroll> &
            pianorolls() const noexcept;

        [[nodiscard]] const std::vector<Waveform> &
            waveforms() const noexcept;

        [[nodiscard]] const std::vector<Problem> &
            problems() const noexcept;

        [[nodiscard]] bool hasError() const noexcept;

        [[nodiscard]] std::optional<DocumentSpan> spanIn(
            std::size_t voice,
            std::size_t begin,
            std::size_t length) const noexcept;

        [[nodiscard]] std::string_view chainOf(
            std::size_t voice) const noexcept;

        [[nodiscard]] std::size_t reparses() const noexcept;

    private:
        struct Segment final
        {
            std::size_t chainBegin = 0;
            std::size_t documentBegin = 0;
            std::size_t length = 0;
        };

        struct Line final
        {
            std::string chain;
            std::string failure;

            std::vector<Segment> segments;

            std::optional<std::vector<std::string>> parts;

            std::size_t notationAt = 0;

            bool pianoroll = false;

            bool waveform = false;

            bool ever = false;

            bool sounding = false;
            Voice voice;
        };

        struct Gathered final
        {
            std::string chain;

            std::vector<Segment> segments;

            std::optional<std::vector<std::string>> parts;

            std::size_t opened = 0;
        };

        void readLine(
            std::string_view line,
            std::size_t number,
            std::size_t lineBegin,
            Gathered &gathering);

        void finish(Gathered &gathering);

        void play(const Gathered &gathering);

        void readForm(std::string_view text, std::size_t number);

        void readBars(std::string_view text, std::size_t number);

        void readPart(std::string_view text, std::size_t number);

        void assemble();

        void refuse(std::size_t number, std::string message);

        NoteWords words;

        std::vector<Line> lines;
        std::vector<Voice> sounding;

        std::vector<Pianoroll> rolls;

        std::vector<Waveform> waves;

        std::vector<std::size_t> soundingLines;

        std::vector<Problem> refusals;

        std::vector<FormUse> formHeld;
        std::size_t formLine = 0;
        bool formEver = false;
        bool formPresent = false;

        std::int64_t barsHeld = 0;
        bool barsEver = false;
        bool barsPresent = false;

        std::optional<std::vector<std::string>> activeParts;

        std::vector<std::string> partNames;

        std::size_t firstPartLine = 0;

        std::string document;

        std::size_t seen = 0;

        std::size_t parsed = 0;

        bool everRead = false;
    };

}
