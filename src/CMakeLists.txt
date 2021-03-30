cmake_minimum_required (VERSION 3.8)

project("ActorCore")

add_library(${PROJECT_NAME} STATIC)
add_library(rf::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

set(THREADS_PREFER_PTHREAD_FLAG ON PARENT_SCOPE)
find_package(Threads REQUIRED)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)

file(GLOB HEADERS 
  ${CMAKE_CURRENT_SOURCE_DIR}/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Actors/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/Actors/*.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Core/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/Core/*.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Ports/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/Ports/*.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/System/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/System/*.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Utils/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/Utils/*.hpp
)

file(GLOB SOURCES 
  ${PROJECT_SOURCE_DIR}/*.cpp
  ${PROJECT_SOURCE_DIR}/Actors/*.cpp
  ${PROJECT_SOURCE_DIR}/Core/*.cpp
  ${PROJECT_SOURCE_DIR}/Ports/*.cpp
  ${PROJECT_SOURCE_DIR}/Ports/*.cpp
  ${PROJECT_SOURCE_DIR}/System/*.cpp
  ${PROJECT_SOURCE_DIR}/Utils/*.cpp
 )

source_group("Header Files" FILES ${HEADERS})
source_group("Source Files" FILES ${SOURCES})

target_sources(${PROJECT_NAME}
  PRIVATE ${HEADERS} ${SOURCES})


target_include_directories(${PROJECT_NAME} 
    PUBLIC ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/Actors
    ${PROJECT_SOURCE_DIR}/Core
    ${PROJECT_SOURCE_DIR}/Ports
    ${PROJECT_SOURCE_DIR}/System
    ${PROJECT_SOURCE_DIR}/Utils
 )

 target_link_libraries(${PROJECT_NAME}
          PUBLIC  
                 nlohmann_json::json
                  Threads::Threads
                  p7
)



