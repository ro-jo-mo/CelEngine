# Delete shader files from the build folder that are no longer needed
cmake_minimum_required(VERSION 3.20)

if (NOT OUT_DIR OR NOT MANIFEST)
    message(FATAL_ERROR "DeleteOld: OUT_DIR and MANIFEST must be set")
endif ()

file(READ "${MANIFEST}" expected_outputs)

# Normalise expected outputs to lower-case paths relative to OUT_DIR
set(expected)
foreach (output IN LISTS expected_outputs)
    file(RELATIVE_PATH rel "${OUT_DIR}" "${output}")
    string(TOLOWER "${rel}" rel)
    list(APPEND expected "${rel}")
endforeach ()

file(GLOB_RECURSE existing LIST_DIRECTORIES false "${OUT_DIR}/*.spv")

foreach (file IN LISTS existing)
    file(RELATIVE_PATH rel "${OUT_DIR}" "${file}")
    string(TOLOWER "${rel}" rel_lower)
    if (NOT rel_lower IN_LIST expected)
        message(STATUS "Removing stale shader: ${rel}")
        file(REMOVE "${file}")
    endif ()
endforeach ()

# Remove any subdirectories left empty
file(GLOB_RECURSE dirs LIST_DIRECTORIES true "${OUT_DIR}/*")
list(SORT dirs ORDER DESCENDING) # children before parents
foreach (dir IN LISTS dirs)
    if (IS_DIRECTORY "${dir}")
        file(GLOB children "${dir}/*")
        if (NOT children)
            file(REMOVE_RECURSE "${dir}") # dir is confirmed empty
        endif ()
    endif ()
endforeach ()
