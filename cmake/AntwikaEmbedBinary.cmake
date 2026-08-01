# Turns a file on disk into a C++ source holding its bytes.
# A library that must not open a file at run time still has to get its
# bytes from somewhere, and a path baked in at configure time is the
# building machine's rather than the running one's -- the reason
# antwika_bundle_app() exists for the assets an application opens.
# Embedding is the answer for a library that has no application to ask.
#
# The generated source is data and nothing else: no function, no
# initialiser that runs, and so nothing for the coverage gate to measure.
# It is written at configure time rather than by a custom command, so it
# needs no build-time tool -- which is what keeps a cross build to MinGW
# from needing a host-built generator.

# Wraps a regular expression so it matches `count` copies of itself.
# CMake's regex engine has no counted repetition, so the pattern is
# spelled out.
function(_antwika_repeat_pattern pattern count out)
    set(repeated "")
    math(EXPR last "${count} - 1")

    foreach(index RANGE ${last})
        string(APPEND repeated "${pattern}")
    endforeach()

    set(${out} "${repeated}" PARENT_SCOPE)
endfunction()

# Generates a C++ source defining one byte array and its length.
#
# INPUT      The file to embed; re-read whenever it changes.
# OUTPUT     The .cpp to write, normally under CMAKE_CURRENT_BINARY_DIR.
# NAMESPACE  The namespace to define the two symbols in.
# SYMBOL     The array's name; the length is <SYMBOL>Size.
#
# The output is a pure function of the input, so two machines building
# the same checkout compile the same bytes.
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

    # Re-running CMake when the file changes is what keeps the generated
    # source from going stale behind a build that looks incremental.
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

    # file(CONFIGURE) leaves an unchanged file alone, timestamp
    # included, so re-running CMake does not rebuild what it wrote.
    file(CONFIGURE OUTPUT ${ARG_OUTPUT} CONTENT "${content}" @ONLY)
endfunction()
