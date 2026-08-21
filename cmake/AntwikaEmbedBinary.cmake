function(_antwika_repeat_pattern pattern count out)
    set(repeated "")
    math(EXPR last "${count} - 1")

    foreach(index RANGE ${last})
        string(APPEND repeated "${pattern}")
    endforeach()

    set(${out} "${repeated}" PARENT_SCOPE)
endfunction()

function(antwika_embed_binary)
    cmake_parse_arguments(ARG "" "INPUT;OUTPUT;NAMESPACE;SYMBOL" "" ${ARGN})

    foreach(required INPUT OUTPUT NAMESPACE SYMBOL)
        if(NOT ARG_${required})
            message(FATAL_ERROR
                "antwika_embed_binary: ${required} is required")
        endif()
    endforeach()

    if(NOT EXISTS ${ARG_INPUT})
        message(FATAL_ERROR
            "antwika_embed_binary: ${ARG_INPUT} does not exist")
    endif()

    set_property(DIRECTORY APPEND
        PROPERTY CMAKE_CONFIGURE_DEPENDS ${ARG_INPUT}
    )

    file(READ ${ARG_INPUT} hex HEX)
    string(LENGTH "${hex}" digits)
    math(EXPR count "${digits} / 2")

    string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," body "${hex}")
    _antwika_repeat_pattern("0x[0-9a-f][0-9a-f]," 12 twelve)
    string(REGEX REPLACE "(${twelve})" "\\1\n        " body "${body}")

    get_filename_component(name ${ARG_INPUT} NAME)
    set(content
"// Generated from ${name} by antwika_embed_binary(); do not edit.
#include <cstddef>
#include <cstdint>

namespace ${ARG_NAMESPACE}
{

    extern const std::uint8_t ${ARG_SYMBOL}[];
    extern const std::size_t ${ARG_SYMBOL}Size;

    const std::uint8_t ${ARG_SYMBOL}[] = {
        ${body}
    };

    const std::size_t ${ARG_SYMBOL}Size = ${count};

} // namespace ${ARG_NAMESPACE}
")

    file(CONFIGURE OUTPUT ${ARG_OUTPUT} CONTENT "${content}" @ONLY)
endfunction()
