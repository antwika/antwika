#include "antwika/companion/FilePetStore.hpp"

#include <utility>

#include "antwika/companion/PetSave.hpp"

namespace antwika::companion
{

    FilePetStore::FilePetStore(std::string path)
        : file(
              std::move(path),
              readCompanionMemory,
              writeCompanionMemory,
              "a companion")
    {
    }

    std::optional<CompanionMemory> FilePetStore::load()
    {
        return file.loadIfPresent();
    }

    void FilePetStore::save(const CompanionMemory &memory)
    {
        file.store(memory);
    }

} // namespace antwika::companion
