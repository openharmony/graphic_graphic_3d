cmake_minimum_required(VERSION 3.18)

set(CORE_ROOT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/../LumeEngine" CACHE PATH "Path engine sdk root dir")

if (NOT TARGET AGPEngine::AGPEngineAPI)
    if (EXISTS "${CORE_ROOT_DIRECTORY}/api/CMakeLists.txt")
        message("Found core engine source in ${CORE_ROOT_DIRECTORY}/api")
        if (NOT TARGET AGPEngine::AGPEngineAPI)
            message("AGPEngine::AGPEngineAPI target does not exist yet, so add core engine from the submodule")
            add_subdirectory("${CORE_ROOT_DIRECTORY}/api" LumeEngineApi)
        endif()
    elseif()
        add_library(AGPEngine::AGPEngineAPI INTERFACE IMPORTED GLOBAL)
        set_target_properties(AGPEngine::AGPEngineAPI PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES ${CORE_ROOT_DIRECTORY}/api/)
    endif()
endif()
