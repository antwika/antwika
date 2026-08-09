#include "antwika/tower_defence/HighScore.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

#include <antwika/config/ConfigDocument.hpp>
#include <antwika/config/FileFormat.hpp>
#include <antwika/config/Format.hpp>
#include <antwika/replay/JsonShapes.hpp>

#include "antwika/tower_defence/ScoreFormatError.hpp"

namespace antwika::tower_defence
{

    namespace
    {
        using antwika::config::FileFormat;
        using antwika::config::FormatSpec;
        using antwika::replay::countShape;

        using ScoreFormat = FileFormat<HighScore, ScoreFormatError>;

        void describeMembers(nlohmann::json &schema)
        {
            schema["required"] = {
                "magic", "bestScore", "bestLevel"}; // GCOVR_EXCL_LINE
            schema["properties"]["bestScore"] = countShape();
            schema["properties"]["bestLevel"] = countShape();
        }

        void encodeMembers(const HighScore &score, nlohmann::json &out)
        {
            out["bestScore"] = score.bestScore;
            out["bestLevel"] = score.bestLevel;
        }

        HighScore decodeMembers(const nlohmann::json &document)
        {
            return HighScore{
                .bestScore =
                    document.at("bestScore").get<std::uint64_t>(),
                .bestLevel =
                    document.at("bestLevel").get<std::size_t>()};
        }

        const ScoreFormat &scoreFormat()
        {
            static const ScoreFormat format(
                FormatSpec<HighScore>{
                    .format =
                        {.magic = kScoreMagic,
                         .version = kScoreFormatVersion},
                    .title = "antwika tower defence score document",
                    .whatFailed = "antwika::tower_defence: a saved high "
                                  "score failed schema validation: ",
                    .members = describeMembers,
                    .encode = encodeMembers,
                    .decode = decodeMembers,
                    .migrations =
                        standardScoreMigrations}); // GCOVR_EXCL_LINE
            return format;
        }
    }

    MigrationChain standardScoreMigrations()
    {
        return MigrationChain({}, kScoreFormatVersion); // GCOVR_EXCL_LINE
    }

    nlohmann::json highScoreToJson(const HighScore &score)
    {
        return scoreFormat().toJson(score);
    }

    HighScore highScoreFromJson(const nlohmann::json &document)
    {
        return scoreFormat().fromJson(document);
    }

    void writeHighScore(const HighScore &score, std::ostream &out)
    {
        scoreFormat().write(score, out);
    }

    HighScore readHighScore(std::istream &in)
    {
        return scoreFormat().read(in);
    }

    HighScore bestOf(const HighScore &best, const HighScore &run)
    {
        if (run.bestScore <= best.bestScore)
        {
            return best;
        }

        return run;
    }

}
