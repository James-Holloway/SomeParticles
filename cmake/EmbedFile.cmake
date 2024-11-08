cmake_minimum_required(VERSION 3.30)

add_executable(EmbedFileUtility cmake/embedfile.cpp)

# Creates a variable named VARNAME in a C file DSTFILE
function(EmbedFile TARGET SRCFILE DSTFILE VARNAME)
    add_custom_command(
            OUTPUT "${DSTFILE}"
            COMMAND EmbedFileUtility "${SRCFILE}" "${DSTFILE}" "${VARNAME}"
            DEPENDS ${SRCFILE}
    )
    target_sources(${TARGET} PRIVATE ${DSTFILE})
endfunction()

# Same as EmbedFile but also adds the embedded file as a source to the target