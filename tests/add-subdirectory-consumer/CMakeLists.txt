cmake_minimum_required(VERSION 3.24)
project(intel_pmu_add_subdirectory_consumer LANGUAGES C)

set(BUILD_TESTING OFF CACHE BOOL "Disable collector tests in this consumer" FORCE)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../.." intel_pmu)

add_executable(intel_pmu_add_subdirectory_consumer ../package-consumer/main.c)
target_link_libraries(intel_pmu_add_subdirectory_consumer PRIVATE intel_pmu::intel_pmu)
