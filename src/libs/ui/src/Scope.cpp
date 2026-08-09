#include "antwika/ui/Scope.hpp"

#include "antwika/ui/Context.hpp"

namespace antwika::ui
{

    Scope::Scope(Context &context) noexcept : context{context}
    {
    }

    Scope::~Scope()
    {
        context.closeContainer();
    }

}
