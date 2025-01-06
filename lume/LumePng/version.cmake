execute_process(COMMAND git log --pretty=format:'%h' -n 1
                OUTPUT_VARIABLE GIT_REV
                WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
                ERROR_QUIET)

# Check whether we got any revision (which isn't
# always the case, e.g. when someone downloaded a zip
# file instead of a checkout)
if ("${GIT_REV}" STREQUAL "")
    set(GIT_REV "N/A")
    set(GIT_DIFF "")
    set(GIT_TAG "N/A")
    set(GIT_BRANCH "N/A")
else()
    execute_process(
        COMMAND git diff --quiet --exit-code
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        RESULT_VARIABLE GIT_DIFF)

    execute_process(
        COMMAND git rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        OUTPUT_VARIABLE GIT_BRANCH)

    execute_process(
        COMMAND git describe --tags --dirty
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        OUTPUT_VARIABLE GIT_TAG ERROR_QUIET)

    string(STRIP "${GIT_REV}" GIT_REV)

    string(SUBSTRING "${GIT_REV}" 1 7 GIT_REV)
    if (GIT_DIFF)
        set(GIT_DIFF "-dirty")
    else()
        set(GIT_DIFF "")
    endif()

    string(STRIP "${GIT_TAG}" GIT_TAG)
    if (NOT GIT_TAG)
        set(GIT_TAG "${GIT_REV}${GIT_DIFF}")
    endif()

    string(STRIP "${GIT_BRANCH}" GIT_BRANCH)
endif()

set(VERSION_STRING
)
set(VERSION "
namespace PNGPlugin {
const char* GetVersionInfo() { return \"GIT_TAG: ${GIT_TAG} GIT_REVISION: ${GIT_REV}${GIT_DIFF} GIT_BRANCH: ${GIT_BRANCH}\"; }
}
")

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/version.cpp)
    file(READ ${CMAKE_CURRENT_SOURCE_DIR}/version.cpp VERSION_)
else()
    set(VERSION_ "")
endif()

if (NOT "${VERSION}" STREQUAL "${VERSION_}")
    file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/version.cpp "${VERSION}")
    message(${VERSION})
endif()