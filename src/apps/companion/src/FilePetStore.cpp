#include "antwika/companion/FilePetStore.hpp"

#include <fstream>
#include <utility>

#include "antwika/companion/PetSave.hpp"
#include "antwika/companion/SaveFormatError.hpp"

namespace antwika::companion
{

    FilePetStore::FilePetStore(std::string path) : path(std::move(path))
    {
    }

    std::optional<CompanionMemory> FilePetStore::load()
    {
        std::ifstream file(path);

        // A file that is not there is not a malformed one.
        // It is a first run, and a first run has no companion yet.
        // Unchecked it would reach the parser as an empty stream.
        // Which reports "you have never had a companion" as corruption.
        if (!file.is_open())
        {
            return std::nullopt;
        }

        return readCompanionMemory(file);
    }

    void FilePetStore::save(const CompanionMemory &memory)
    {
        std::ofstream file(path);
        if (!file.is_open())
        {
            throw SaveFormatError(
                "antwika::companion: could not open a companion to "
                "write: " + path);
        }

        writeCompanionMemory(memory, file);

        // Flushed here rather than by the destructor, which cannot say.
        // A full disk fails on the flush, not on the open.
        file.flush();
        if (!file)
        {
            throw SaveFormatError(
                "antwika::companion: could not write a companion: "
                + path);
        }
    }

} // namespace antwika::companion
