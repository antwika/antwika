#include "antwika/console/ConsolePicture.hpp"

#include <utility>

namespace antwika::console
{

    ConsolePicture::ConsolePicture(Size canvas) : area(canvas)
    {
    }

    Size ConsolePicture::canvas() const noexcept
    {
        return area;
    }

    void ConsolePicture::set(DrawList commands)
    {
        picture = std::move(commands);
    }

    const DrawList &ConsolePicture::commands() const noexcept
    {
        return picture;
    }

}
