#include "antwika/music_editor/Score.hpp"

#include <array>
#include <cstddef>
#include <string>

#include <antwika/notation/NotationError.hpp>
#include <antwika/notation/ParsePattern.hpp>
#include <antwika/pattern/PatternError.hpp>
#include <antwika/pattern/Patterns.hpp>

namespace antwika::music_editor
{

    namespace
    {
        [[nodiscard]] bool isBlank(const std::string &line) noexcept
        {
            for (const auto letter : line)
            {
                if (letter != ' ' && letter != '\t')
                {
                    return false;
                }
            }

            return true;
        }
    } // namespace

    Score::Score() : words(kNote)
    {
        tracks.reserve(kTrackCount);

        for (std::size_t track = 0; track < kTrackCount; ++track)
        {
            tracks.push_back(
                Track{
                    .source = std::string{},
                    .failure = std::string{},
                    .playing = pattern::silence()});
        }
    }

    void Score::update(
        const std::array<std::string, kTrackCount> &lines)
    {
        for (std::size_t track = 0; track < kTrackCount; ++track)
        {
            auto &held = tracks[track];

            if (held.source == lines[track] && parsed > 0)
            {
                continue;
            }

            held.source = lines[track];
            ++parsed;

            if (isBlank(held.source))
            {
                held.playing = pattern::silence();
                held.failure.clear();

                continue;
            }

            try
            {
                held.playing =
                    notation::parsePattern(held.source, words);

                held.failure.clear();
            }
            catch (const notation::NotationError &refused)
            {
                // The line keeps playing whatever it last did.
                // Half a bracket is typed on the way to a whole one.
                held.failure = refused.what();
            }
            catch (const pattern::PatternError &refused)
            {
                // It read cleanly and asked for something impossible.
                // The algebra refused it, and that reads the same here.
                held.failure = refused.what();
            }
        }
    }

    const Pattern &Score::playing(const std::size_t track) const
    {
        return tracks[track].playing;
    }

    const std::string &Score::error(
        const std::size_t track) const noexcept
    {
        return tracks[track].failure;
    }

    bool Score::hasError() const noexcept
    {
        for (const auto &track : tracks)
        {
            if (!track.failure.empty())
            {
                return true;
            }
        }

        return false;
    }

    std::size_t Score::reparses() const noexcept
    {
        return parsed;
    }

} // namespace antwika::music_editor
