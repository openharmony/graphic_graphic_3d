set(LIB_NAME MetaEnvironmentSetup)

aux_source_directory(. SOURCE_FILES)

add_library(${LIB_NAME} STATIC ${SOURCE_FILES})

target_include_directories(${LIB_NAME} INTERFACE .)

set_target_properties(${LIB_NAME} PROPERTIES CXX_STANDARD 17
                                             CXX_STANDARD_REQUIRED ON
                                             CXX_STANDARD_EXTENSIONS OFF
        )

target_compile_definitions(${LIB_NAME} PUBLIC CORE_DYNAMIC=1
                                              PRIVATE NOMINMAX
        )

target_link_libraries(${LIB_NAME} PRIVATE AGPEngine::AGPEngineAPI
        AGPBase::AGPBaseAPI
        MetaObject::API)

add_library(MetaObject::EnvironmentSetup ALIAS ${LIB_NAME} )