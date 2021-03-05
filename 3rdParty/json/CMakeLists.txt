cmake_minimum_required (VERSION 3.13)

project("json" LANGUAGES CXX)

## TARGET
## create target and add include path
if (NOT TARGET ${PROJECT_NAME})
    add_library(${PROJECT_NAME} INTERFACE)
    add_library(nlohmann_json::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
endif()

file(GLOB SOURCES 
    ${PROJECT_SOURCE_DIR}/*.hpp
 )

source_group("Source Files" FILES ${SOURCES})

#target_sources(${PROJECT_NAME}
#  PRIVATE  ${SOURCES})


target_include_directories(${PROJECT_NAME} 
    INTERFACE ${PROJECT_SOURCE_DIR}
 )


