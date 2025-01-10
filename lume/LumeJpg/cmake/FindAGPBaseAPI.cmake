cmake_minimum_required(VERSION 3.18)

set(BASE_ROOT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/../LumeBase" CACHE PATH "Path base root dir")

if (EXISTS "${BASE_ROOT_DIRECTORY}/CMakeLists.txt")
    message("Found base source in ${BASE_ROOT_DIRECTORY}")
    if (NOT TARGET AGPBase::AGPBaseAPI)
        message("AGPBase::AGPBaseAPI target does not exist yet, so add base from the submodule")
        add_subdirectory("${BASE_ROOT_DIRECTORY}/" LumeBase)
    endif()
endif()

if (NOT TARGET AGPBase::AGPBaseAPI)
    #add_library(AGPBase::AGPBaseAPI INTERFACE IMPORTED GLOBAL)
    #set_target_properties(AGPBase::AGPBaseAPI PROPERTIES
    #    INTERFACE_INCLUDE_DIRECTORIES ${BASE_ROOT_DIRECTORY}/api/)
endif()
