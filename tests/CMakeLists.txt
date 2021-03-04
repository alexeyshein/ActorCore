
cmake_minimum_required (VERSION 3.11)

project("ActorCoreTest")


add_executable(${PROJECT_NAME}) 
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)

file(GLOB_RECURSE TEST_SOURCES LIST_DIRECTORIES false *.h *.hpp *.cpp)

target_sources(${PROJECT_NAME}  PRIVATE ${TEST_SOURCES})

add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PUBLIC 
							rf::Core 
							#gtest
							)

# if(WITH_CameraBasler)
# 	add_definitions(-DBUILD_CameraBasler)
# endif()


