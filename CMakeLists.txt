cmake_minimum_required (VERSION 3.8)

project (ActorCoreSolution)

# Optional  components
# ===================================================
option(WITH_${PROJECT_NAME}_Samples "Include CoreInterchange Samples" OFF)
option(WITH_${PROJECT_NAME}_Tests  "Include CoreInterchange Unit Tests" OFF)

add_subdirectory (3rdParty)
add_subdirectory (src)
# Включите подпроекты.
if(WITH_${PROJECT_NAME}_Samples)
	add_definitions(-DBUILD_Samples)
	add_subdirectory (samples)
endif()

if(WITH_${PROJECT_NAME}_Tests)
	add_definitions(-DBUILD_Test)
	add_subdirectory (tests)
endif()


